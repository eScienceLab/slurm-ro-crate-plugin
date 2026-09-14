// -----------------------------------------------------------------------------
// SPANK C headers

extern "C" {
#include <slurm/spank.h>

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
#include <ro-crate.hpp>

using ROCrate = rocrate::ROCrate;

// -----------------------------------------------------------------------------
// Function prototypes

std::filesystem::path get_output_dir(int ac, char *av[]);
void generate_crate(spank_t sp);

// -----------------------------------------------------------------------------
// Globals for passing between callbacks

std::filesystem::path output_dir;

// -----------------------------------------------------------------------------
// SPANK skeleton

int slurm_spank_init(spank_t, int ac, char *av[]) {
  slurm_debug("slurm_ro_crate_plugin: -> init context=%d", spank_context());
  output_dir = get_output_dir(ac, av);
  if (output_dir.empty()) {
    slurm_error("slurm_ro_crate_plugin: output_dir not set in plugstack.conf");
    return SLURM_ERROR;
  }
  return SLURM_SUCCESS;
}

int slurm_spank_job_prolog(spank_t sp, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> job_prolog remote=%d", spank_remote(sp));
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

int slurm_spank_task_init_privileged(spank_t, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> task_init_privileged");
  return SLURM_SUCCESS;
}

int slurm_spank_task_init(spank_t, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> task_init");
  return SLURM_SUCCESS;
}

int slurm_spank_task_post_fork(spank_t, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> task_post_fork");
  return SLURM_SUCCESS;
}

int slurm_spank_task_exit(spank_t sp, int, char **) {
  int status;
  slurm_debug("slurm_ro_crate_plugin: -> task_exit");
  if (spank_get_item(sp, S_TASK_EXIT_STATUS, &status) == ESPANK_SUCCESS)
    slurm_debug("slurm_ro_crate_plugin: exit_status=%d", status);
  return SLURM_SUCCESS;
}

int slurm_spank_exit(spank_t sp, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> exit remote=%d", spank_remote(sp));
  if (!spank_remote(sp)) return SLURM_SUCCESS;

  generate_crate(sp);
  return SLURM_SUCCESS;
}

int slurm_spank_job_epilog(spank_t sp, int, char **) {
  slurm_debug("slurm_ro_crate_plugin: -> job_epilog remote=%d", spank_remote(sp));
  return SLURM_SUCCESS;
}

// -----------------------------------------------------------------------------
// Crate generation functions

std::filesystem::path get_output_dir(int ac, char *av[]) {
  for (int i = 0; i < ac; i++) {
    if (std::strncmp(av[i], "output_dir=", 11) == 0) {
      return av[i] + 11;
    }
  }
  return {};
}

void generate_crate(spank_t sp) {
  // Get the Job ID of the run
  uint32_t job_id;
  if (spank_get_item(sp, S_JOB_ID, &job_id) != ESPANK_SUCCESS) return;

  // Create an output directory to hold the metadata file
  auto dir = output_dir / std::to_string(job_id);
  std::filesystem::create_directories(dir);

  // Create the RO-Crate object
  ROCrate crate;

  // Write out the json
  crate.writeOut(dir / "ro-crate-metadata.json");
  slurm_debug("slurm_ro_crate_plugin: wrote %s/ro-crate-metadata.json", dir.c_str());
}
