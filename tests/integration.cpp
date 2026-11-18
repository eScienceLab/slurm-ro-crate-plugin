// Integration test for the installed slurm-ro-crate-plugin SPANK plugin.
//
// This assumes a working Slurm environment is already present and reachable
// (sbatch/srun on PATH, slurmd/slurmctld running, the plugin already built
// and registered in plugstack.conf)

#include <catch2/catch_test_macros.hpp>
#include <json.hpp>
#include <reproc++/run.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

std::string output_dir = "/tmp/spank-crates";

std::pair<int, std::string> run_command(const std::vector<std::string>& args) {
  std::string output;
  auto [status, ec] = reproc::run(
    args,
    reproc::options{},
    reproc::sink::string(output),
    reproc::sink::string(output)
  );
  if (ec) {
    throw std::runtime_error("command failed to run: " + ec.message());
  }
  return {status, output};
}

std::string submit_job(const std::vector<std::string>& params) {
  // Run the command and capture the output
  auto [status, output] = run_command({"sbatch", "--parsable", "--wait", params[0]});

  // Check for errors
  if (status != 0) {
    throw std::runtime_error("sbatch failed: " + output);
  }
  
  // Strip the newline from the output (job ID) and return it
  output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
  return output;
}

void validate_crate(const std::vector<std::string>& params) {
  // If available, run the RO-Crate validator on the generated crate and assert that it passes.
  if (run_command({"which", "uvx"}).first != 0) {
    SKIP("uvx not available, skipping crate validator check");
  }

  // Form the command to run the validator
  std::filesystem::path crate_path = output_dir + "/" + params[0] + "/" + "ro-crate-metadata.json";

  // Run the validator and capture the output
  auto [validator_status, validator_output] = run_command({
    "uvx", "--from", "roc-validator", "rocrate-validator", "validate", crate_path.string()
  });

  // Check for errors
  INFO(validator_output);
  REQUIRE(validator_status == 0);
}

TEST_CASE("a submitted job produces a valid RO-Crate", "[integration]") {
  std::string fixtures_dir = std::string(TEST_SOURCE_DIR) + "/tests/fixtures";
  
  // Submit a trivial job and get its job ID.
  std::string job_id = submit_job({fixtures_dir + "/single-task.slurm"});
  REQUIRE_FALSE(job_id.empty());

  // Assert that the RO-Crate file exists and is readable.
  std::filesystem::path crate_path =
    output_dir + "/" + job_id + "/" + "ro-crate-metadata.json";
  std::cout << "Checking for RO-Crate at: " << crate_path << std::endl;
  REQUIRE(std::filesystem::exists(crate_path));

  std::ifstream file(crate_path);
  REQUIRE(file.good());

  // Assert that the RO-Crate file is valid JSON and contains the required fields.
  nlohmann::json crate = nlohmann::json::parse(file);

  REQUIRE(crate.contains("@context"));
  REQUIRE(crate.contains("@graph"));
  REQUIRE(crate.at("@graph").is_array());

  // Assert that the RO-Crate contains a root dataset and a root descriptor.
  bool has_root_dataset = false;
  bool has_root_descriptor = false;

  for (const auto &entity : crate.at("@graph")) {
    if (entity.at("@id") == "./") has_root_dataset = true;
    if (entity.at("@id") == "ro-crate-metadata.json") has_root_descriptor = true;
  }

  REQUIRE(has_root_dataset);
  REQUIRE(has_root_descriptor);

  // Assert that the RO-Crate contains a job_id and user_id in the root dataset.
  bool has_job = false;
  std::string job_tag = "#job" + job_id;
  for (const auto &entity : crate.at("@graph")) {
    if (entity.at("@id") == job_tag) {
      has_job = true;
      REQUIRE(entity.at("@type") == "CreateAction");
      REQUIRE(entity.at("identifier") == job_id);
    } 
  }
  REQUIRE(has_job);

  bool has_agent = false;
  for (const auto &entity : crate.at("@graph")) {
    if (entity.at("@id") == "#agent") {
      has_agent = true;
      REQUIRE(entity.at("@type") == "Person");
      REQUIRE(entity.contains("identifier"));
    } 
  }

  // Validate the crate using the RO-Crate validator if available.
  validate_crate({job_id});
}

TEST_CASE("Multi-task job produces a valid RO-Crate", "[integration]") {
  // Smoke test: multiple tasks hit the same crate file concurrently via
  // task_exit, each going through ROCrateManager's flock-guarded read/write.
  // This checks that doesn't corrupt the crate -- it does not assert
  // anything about what metadata (if any) gets written per task.
  std::string fixtures_dir = std::string(TEST_SOURCE_DIR) + "/tests/fixtures";

  // Submit a multi-task job and get its job ID.
  std::string job_id = submit_job({fixtures_dir + "/multi-task.slurm"});
  REQUIRE_FALSE(job_id.empty());

  // Assert that the RO-Crate file exists and is readable.
  std::filesystem::path crate_path =
    output_dir + "/" + job_id + "/" + "ro-crate-metadata.json";
  std::cout << "Checking for RO-Crate at: " << crate_path << std::endl;
  REQUIRE(std::filesystem::exists(crate_path));

  std::ifstream file(crate_path);
  REQUIRE(file.good());

  // Assert that the RO-Crate file is valid JSON and contains the required fields.
  nlohmann::json crate = nlohmann::json::parse(file);

  REQUIRE(crate.contains("@context"));
  REQUIRE(crate.contains("@graph"));
  REQUIRE(crate.at("@graph").is_array());

  // Assert that the RO-Crate contains a root dataset and a root descriptor.
  bool has_root_dataset = false;
  bool has_root_descriptor = false;

  for (const auto &entity : crate.at("@graph")) {
    if (entity.at("@id") == "./") has_root_dataset = true;
    if (entity.at("@id") == "ro-crate-metadata.json") has_root_descriptor = true;
  }

  REQUIRE(has_root_dataset);
  REQUIRE(has_root_descriptor);

  // Validate the crate using the RO-Crate validator if available.
  validate_crate({job_id});
}
