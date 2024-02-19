#!/bin/bash
#
#SBATCH -t 0-0:02
#SBATCH -N 3
#SBATCH -n 12
#SBATCH -o slurm.%N.%j.out
#SBATCH -e slurm.%N.%j.err

mpiexec ./1705258164738.a.out parametry.txt result.txt