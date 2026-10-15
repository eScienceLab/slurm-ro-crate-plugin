#!/bin/bash --login
#SBATCH --ntasks 2
srun --ntasks="$SLURM_NTASKS" hostnamectl
