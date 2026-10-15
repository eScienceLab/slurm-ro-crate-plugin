#!/bin/bash --login
#SBATCH --ntasks 2
#SBATCH --output=/dev/null
srun --ntasks="$SLURM_NTASKS" hostnamectl
