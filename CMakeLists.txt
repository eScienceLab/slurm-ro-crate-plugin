cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
project (slurm-ro-crate-plugin CXX)

include(CheckIncludeFiles)
include(FindPackageHandleStandardArgs)
include(CTest)

find_path(SLURM_INCLUDE_DIR NAMES slurm/slurm.h)
find_library(SLURM_LIBRARY NAMES libslurm.so)
find_package_handle_standard_args(SLURM DEFAULT_MSG SLURM_LIBRARY SLURM_INCLUDE_DIR)
if (NOT SLURM_FOUND)
  MESSAGE(FATAL_ERROR "SLURM library could not be found")
else (NOT SLURM_FOUND)
  set (SLURM_LIBRARIES ${SLURM_LIBRARY})
  set (SLURM_INCLUDE_DIRS ${SLURM_INCLUDE_DIR})
  find_path(SLURM_MODULES_DIR NAMES slurm
          PATHS
          /usr/lib64/slurm
          /usr/lib/x86_64-linux-gnu/slurm
          DOC "Directory containing SLURM extensions."
  )
  if (NOT SLURM_MODULES_DIR)
    message(FATAL_ERROR "SLURM extensions directory could not be found")
  endif (NOT SLURM_MODULES_DIR)
endif (NOT SLURM_FOUND)
mark_as_advanced (SLURM_LIBRARIES SLURM_INCLUDE_DIRS)

add_library (slurm-ro-crate-plugin MODULE slurm-ro-crate-plugin.cpp)
include_directories (BEFORE ${SLURM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set_target_properties (slurm-ro-crate-plugin PROPERTIES PREFIX "" SUFFIX "" OUTPUT_NAME "slurm-ro-crate-plugin.so")
target_compile_options(slurm-ro-crate-plugin PRIVATE -Wall -Wextra -O2 -pedantic)

install(TARGETS slurm-ro-crate-plugin LIBRARY DESTINATION lib/slurm)

# -----------------------------------------------------------------------------
# Tests
#
# The integration test assumes a working Slurm environment is already present
# and reachable (sbatch on PATH, daemons running, this plugin already built
# and registered in plugstack.conf) -- it submits a real job and checks the
# resulting RO-Crate, rather than standing up the Slurm stack itself.

if (BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.16.0
    FIND_PACKAGE_ARGS 3
  )
  FetchContent_MakeAvailable(Catch2)

  set(REPROC++ ON CACHE BOOL "" FORCE)
  FetchContent_Declare(reproc
    GIT_REPOSITORY https://github.com/DaanDeMeyer/reproc.git
    GIT_TAG        14.2.8
  )
  FetchContent_MakeAvailable(reproc)

  add_executable(slurm-ro-crate-plugin_tests
    tests/integration.cpp
  )

  target_compile_features(slurm-ro-crate-plugin_tests PRIVATE cxx_std_20)

  target_link_libraries(slurm-ro-crate-plugin_tests
    PRIVATE
      Catch2::Catch2WithMain
      reproc++
  )

  include(Catch)
  catch_discover_tests(slurm-ro-crate-plugin_tests)
endif()
