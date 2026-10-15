// -----------------------------------------------------------------------------
// SPANK C headers

extern "C" {
#include <slurm/spank.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

extern const char plugin_name[] = "slurm_ro_crate_plugin";
extern const char plugin_type[] = "spank";
extern const unsigned int plugin_version = SLURM_VERSION_NUMBER;
extern const unsigned int spank_plugin_version = 1;
}

// -----------------------------------------------------------------------------
// Library headers

#include <filesystem>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <ro-crate.hpp>

using ROCrate = rocrate::ROCrate;
using Entity = rocrate::Entity;

// -----------------------------------------------------------------------------
// Function prototypes

std::filesystem::path get_output_dir(int ac, char *av[]);
uint32_t get_job_id(spank_t sp);

// -----------------------------------------------------------------------------
// RO-Crate read / write boilerplate
// Multiple tasks may be spawned for a single job but need to write to a single
// RO-Crate file. This function handles the reading and writing of the RO-Crate
// file for a given job, per callback, and uses flock to ensure that only one
// task is writing to the file at a time.

class ROCrateManager {
  public:
    ROCrateManager(spank_t sp, int ac, char *av[]) {
      // Get the job id, form the full crate path
      uint32_t job_id = get_job_id(sp);
      std::filesystem::path output_dir = get_output_dir(ac, av);
      crate_path = output_dir / std::to_string(job_id) / "ro-crate-metadata.json";

      // Ensure the path exists
      std::filesystem::create_directories(crate_path.parent_path());

      // Check if the crate exists, if not, create a new one
      bool crate_exists = std::filesystem::exists(crate_path); 

      if (!crate_exists) {
        crate = ROCrate();
        crate.writeOut(crate_path);
      }

      // Acquire a lock on the crate file to prevent concurrent writes
      lock_fd = open(crate_path.c_str(), O_RDWR | O_CREAT, 0666);
      if (lock_fd == -1) {
        throw std::runtime_error("Failed to open crate file for locking");
      }

      // Block until we can acquire the lock
      while (flock(lock_fd, LOCK_EX) == -1) {
        if (errno != EINTR) {
          close(lock_fd);
          throw std::runtime_error("Failed to acquire lock on crate file");
        }
      }

      // Read in the crate
      crate.readIn(crate_path);
    }

    ~ROCrateManager() {
      // Write out the crate and release the lock
      try {
        crate.writeOut(crate_path);
      } catch (const std::exception &e) {
        slurm_error("slurm_ro_crate_plugin: Failed to write RO-Crate: %s", e.what());
      }

      // Release the lock and close the file descriptor
      close(lock_fd);
    }

    ROCrate crate;

  private:
    std::filesystem::path crate_path;
    int lock_fd;
};

// -----------------------------------------------------------------------------
// SPANK skeleton

int slurm_spank_init(spank_t, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> init context=%d", spank_context());
  return SLURM_SUCCESS;
}

int slurm_spank_job_prolog(spank_t sp, int ac, char *av[]) {
  slurm_debug("slurm_ro_crate_plugin: -> job_prolog remote=%d", spank_remote(sp));

  try {
    ROCrateManager manager(sp, ac, av);
    // Populate manager.crate here
  } catch (const std::exception &e) {
    slurm_error("slurm_ro_crate_plugin: Failed to initialize RO-Crate: %s", e.what());
    return SLURM_ERROR;
  }

  return SLURM_SUCCESS;
}

int slurm_spank_init_post_opt(spank_t sp, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> init_post_opt remote=%d", spank_remote(sp));
  return SLURM_SUCCESS;
}

int slurm_spank_local_user_init(spank_t, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> local_user_init");
  return SLURM_SUCCESS;
}

int slurm_spank_user_init(spank_t sp, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> user_init remote=%d", spank_remote(sp));
  return SLURM_SUCCESS;
}

int slurm_spank_task_init_privileged(spank_t sp, int ac, char *av[]) {
  slurm_debug("slurm_ro_crate_plugin: -> task_init_privileged");

  try {
    ROCrateManager manager(sp, ac, av);
    // Populate manager.crate here
  } catch (const std::exception &e) {
    slurm_error("slurm_ro_crate_plugin: %s", e.what());
    return SLURM_ERROR;
  }

  return SLURM_SUCCESS;
}

int slurm_spank_task_init(spank_t, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> task_init");
  return SLURM_SUCCESS;
}

int slurm_spank_task_post_fork(spank_t sp, int ac, char *av[]) {
  slurm_debug("slurm_ro_crate_plugin: -> task_post_fork");

  try {
    ROCrateManager manager(sp, ac, av);
    // Populate manager.crate here
  } catch (const std::exception &e) {
    slurm_error("slurm_ro_crate_plugin: %s", e.what());
    return SLURM_ERROR;
  }

  return SLURM_SUCCESS;
}

int slurm_spank_task_exit(spank_t sp, int ac, char *av[]) {
  int status;
  slurm_debug("slurm_ro_crate_plugin: -> task_exit");

  try {
    ROCrateManager manager(sp, ac, av);

    // Get task and step IDs, exit status
    int task_id = 0;
    if (spank_get_item(sp, S_TASK_ID, &task_id) != ESPANK_SUCCESS) {
      slurm_error("slurm_ro_crate_plugin: Failed to get task ID");
      return SLURM_ERROR;
    }

    int step_id = 0;
    if (spank_get_item(sp, S_JOB_STEPID, &step_id) != ESPANK_SUCCESS) {
      slurm_error("slurm_ro_crate_plugin: Failed to get step ID");
      return SLURM_ERROR;
    }

    uint32_t status = 0;
    if (spank_get_item(sp, S_TASK_EXIT_STATUS, &status) != ESPANK_SUCCESS) {
      slurm_error("slurm_ro_crate_plugin: Failed to get exit status");
      return SLURM_ERROR;
    }

    // Add the IDs and exit status to the RO-Crate
    Entity task({"CreateAction"});
    task.set("id", std::to_string(task_id));
    task.set("step_id", std::to_string(step_id));
    task.set("exit_status", std::to_string(status));

    manager.crate.addEntity("#" + std::to_string(step_id) + "_" + std::to_string(task_id), task);
  } catch (const std::exception &e) {
    slurm_error("slurm_ro_crate_plugin: %s", e.what());
    return SLURM_ERROR;
  }

  if (spank_get_item(sp, S_TASK_EXIT_STATUS, &status) == ESPANK_SUCCESS)
    slurm_debug("slurm_ro_crate_plugin: exit_status=%d", status);
  
  return SLURM_SUCCESS;
}

int slurm_spank_exit(spank_t sp, int ac, char *av[]) {
  slurm_debug("slurm_ro_crate_plugin: -> exit remote=%d", spank_remote(sp));
  if (!spank_remote(sp)) return SLURM_SUCCESS;

  try {
    ROCrateManager manager(sp, ac, av);
    // Populate manager.crate here
  } catch (const std::exception &e) {
    slurm_error("slurm_ro_crate_plugin: %s", e.what());
    return SLURM_ERROR;
  }
  return SLURM_SUCCESS;
}

int slurm_spank_job_epilog(spank_t sp, int ac, char *av[]) {
  slurm_debug("slurm_ro_crate_plugin: -> job_epilog remote=%d", spank_remote(sp));

  try {
    ROCrateManager manager(sp, ac, av);
    // Populate manager.crate here
  } catch (const std::exception &e) {
    slurm_error("slurm_ro_crate_plugin: %s", e.what());
    return SLURM_ERROR;
  }
  return SLURM_SUCCESS;
}

// -----------------------------------------------------------------------------
// Helpers

std::filesystem::path get_output_dir(int ac, char *av[]) {
  // Look for the output_dir argument in the plugstack.conf file
  for (int i = 0; i < ac; i++) {
    if (std::strncmp(av[i], "output_dir=", 11) == 0) {
      return av[i] + 11;
    }
  }

  // If we get here, the output_dir was not set in plugstack.conf
  throw std::runtime_error("output_dir not set in plugstack.conf");
}

uint32_t get_job_id(spank_t sp) {
  uint32_t job_id;
  if (spank_get_item(sp, S_JOB_ID, &job_id) != ESPANK_SUCCESS) {
    throw std::runtime_error("Failed to get job ID");
  }
  return job_id;
}
