# SPANK RO-Crate

This repository contains a SLURM plugin (SPANK) which enables the automatic generation of RO-Crate metadata for jobs submitted to a SLURM workload manager. The plugin captures job information and outputs it in the RO-Crate format.

The intention is to focus on the gathering of 'green metrics' and in particular to support aspects of applying for green standard certification such as the following requirement of GreenDiSCL

```
## Inventory of computing workflow hotspots

There is an inventory of the biggest resource users (e.g. based on number of jobs, longest jobs, highest total CPU time, most memory used) on the RCI. It can be individual users with particularly high usage, or the most popular widely used pipelines.
```

## Repo structure

The repository is structured as follows:

spank-ro-crate/
-> src # Source code for the SPANK plugin
-> tests # Test cases for the plugin
-> examples # Example usage and sample RO-Crate outputs
-> ./ # Root directory containing README and other configuration files
-> build # Build directory for compiling the plugin (empty by default)

## Installation
To compile the SPANK RO-Crate plugin, follow these steps:

```bash
cmake -B build
```

To install the plugin, run the following command:

```bash
sudo cmake --install build
```

Then configure the slurm instance to load the plugin by adding the following line to the `plugstack.conf` file

```diff
+ optional /usr/lib/slurm/spank-ro-crate.so output_dir=/tmp/spank-ro-crates
```

and create the output directory with appropriate permissions:

```bash
sudo mkdir -p /tmp/spank-ro-crates
sudo chmod 777 /tmp/spank-ro-crates
```



