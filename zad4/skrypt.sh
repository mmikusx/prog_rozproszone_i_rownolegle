#!/bin/bash
#
#SBATCH -t 0-0:03 # time (D-HH:MM)
#SBATCH -p OMP    # partycja OMP
#SBATCH -c 7      # liczba rdzeni //////// tu zmieniamy na ilu chcemy odpalać rdzeniach (max 8 tak by było to i>
#SBATCH -e slurm.%N.%j.err # STDERRs}n

if [ -n "$SLURM_CPUS_PER_TASK" ]; then
  omp_threads=${SLURM_CPUS_PER_TASK}
else
  omp_threads=1
fi


export OMP_NUM_THREADS=${omp_threads}
echo "Value of OMP_NUM_THREADS: $OMP_NUM_THREADS"

./1706016662865.a.out 192