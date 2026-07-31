# Slurm RO-Crate Plugin

This repository contains a Slurm plugin (SPANK) which enables the automatic generation of RO-Crate metadata for jobs submitted to a Slurm workload manager. The plugin captures job information and outputs it in the RO-Crate format.

The intention is to focus on the gathering of 'green metrics' and in particular to support aspects of applying for green standard certification such as the following requirement of GreenDiSC:

```
## Inventory of computing workflow hotspots

There is an inventory of the biggest resource users (e.g. based on number of jobs, longest jobs, highest total CPU time, most memory used) on the RCI. It can be individual users with particularly high usage, or the most popular widely used pipelines.
```

## Prerequisites

* CMake >= 3.10
* A C++20 compiler
* Slurm development headers (`libslurm-dev` on Debian/Ubuntu)

## Repo structure

The repository is structured as follows:

slurm-ro-crate-plugin/
-> slurm-ro-crate-plugin.cpp # Source code for the SPANK plugin
-> lib # Bundled third-party headers
-> reference # Reference material for the plugin
-> ./ # Root directory containing README and other configuration files
-> build # Build directory for compiling the plugin (empty by default)

## Installation
To compile the Slurm RO-Crate plugin, follow these steps:

```bash
cmake -B build
```

To install the plugin, run the following command:

```bash
sudo cmake --install build
```

Then configure the Slurm instance to load the plugin by adding the following line to the `plugstack.conf` file

```diff
+ required /usr/local/lib/slurm/slurm-ro-crate-plugin.so output_dir=/tmp/spank-crates
```
