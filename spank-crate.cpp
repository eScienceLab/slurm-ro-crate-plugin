// -----------------------------------------------------------------------------
// SPANK C headers

extern "C" {
#include <slurm/spank.h>

extern const char plugin_name[] = "spank_crate";
extern const char plugin_type[] = "spank";
extern const unsigned int plugin_version = SLURM_VERSION_NUMBER;
extern const unsigned int spank_plugin_version = 1;
}

// -----------------------------------------------------------------------------
// Library headers

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// -----------------------------------------------------------------------------
// Function prototypes

void generate_crate();

// -----------------------------------------------------------------------------
// SPANK skeleton

int slurm_spank_init(spank_t, int, char **) {
  slurm_debug("spank_crate: -> init context=%d", spank_context());
  return SLURM_SUCCESS;
}

int slurm_spank_job_prolog(spank_t sp, int, char **) {
  slurm_debug("spank_crate: -> job_prolog remote=%d", spank_remote(sp));
  return SLURM_SUCCESS;
}

int slurm_spank_init_post_opt(spank_t sp, int, char **) {
  slurm_debug("spank_crate: -> init_post_opt remote=%d", spank_remote(sp));
  return SLURM_SUCCESS;
}

int slurm_spank_local_user_init(spank_t, int, char **) {
  slurm_debug("spank_crate: -> local_user_init");
  return SLURM_SUCCESS;
}

int slurm_spank_user_init(spank_t sp, int, char **) {
  slurm_debug("spank_crate: -> user_init remote=%d", spank_remote(sp));
  return SLURM_SUCCESS;
}

int slurm_spank_task_init_privileged(spank_t, int, char **) {
  slurm_debug("spank_crate: -> task_init_privileged");
  return SLURM_SUCCESS;
}

int slurm_spank_task_init(spank_t, int, char **) {
  slurm_debug("spank_crate: -> task_init");
  return SLURM_SUCCESS;
}

int slurm_spank_task_post_fork(spank_t, int, char **) {
  slurm_debug("spank_crate: -> task_post_fork");
  return SLURM_SUCCESS;
}

int slurm_spank_task_exit(spank_t sp, int, char **) {
  int status;
  slurm_debug("spank_crate: -> task_exit");
  if (spank_get_item(sp, S_TASK_EXIT_STATUS, &status) == ESPANK_SUCCESS)
    slurm_debug("spank_crate: exit_status=%d", status);
  return SLURM_SUCCESS;
}

int slurm_spank_exit(spank_t sp, int, char **) {
  slurm_debug("spank_crate: -> exit remote=%d", spank_remote(sp));
  if (!spank_remote(sp)) return SLURM_SUCCESS;

  generate_crate();
  return SLURM_SUCCESS;
}

int slurm_spank_job_epilog(spank_t sp, int, char **) {
  slurm_debug("spank_crate: -> job_epilog remote=%d", spank_remote(sp));
  return SLURM_SUCCESS;
}

// -----------------------------------------------------------------------------
// 

void generate_crate() {
  json j;
  j["pi"] = 3.141;
  j["happy"] = true;
  j["name"] = "Niels";
  j["nothing"] = nullptr;
  j["answer"]["everything"] = 42;
  j["list"] = { 1, 0, 2 };
  j["object"] = { {"currency", "USD"}, {"value", 42.99} };
  
  slurm_debug("spank_crate: %s", j.dump().c_str());
  return;
}
