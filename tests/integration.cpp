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

std::string submit_job(const std::vector<std::string>& params) {
  // Form the sbatch command
  std::vector<std::string> args = {
    "sbatch", "--parsable", "--wait"
  };
  args.insert(args.end(), params.begin(), params.end());

  // Run the command and capture the output
  std::string output;
  auto [status, ec] = reproc::run(
    args,
    reproc::options{},
    reproc::sink::string(output),
    reproc::sink::null
    );

  // Check for errors
  if (ec || status != 0) {
    throw std::runtime_error("sbatch failed: " + (ec ? ec.message() : output));
  }

  // Strip the newline from the output (job ID) and return it
  output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
  return output;
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
}

TEST_CASE("Multi-task job produces a valid RO-Crate with multiple CreateAction entities", "[integration]") {
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

  nlohmann::json crate = nlohmann::json::parse(file);
  REQUIRE(crate.contains("@graph"));

  // Assert that there are multiple CreateAction entities in the RO-Crate.
  std::vector<std::string> task_entity_ids;
  for (const auto &entity : crate.at("@graph")) {
    if (entity.at("@type") == "CreateAction")
      task_entity_ids.push_back(entity.at("@id").get<std::string>());
  }

  REQUIRE(task_entity_ids.size() == 3);
}
