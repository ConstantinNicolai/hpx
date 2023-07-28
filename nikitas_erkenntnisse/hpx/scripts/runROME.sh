#!/usr/bin/env bash

#SBATCH -p rome
#SBATCH -w ziti-rome2,ziti-rome3
#SBATCH -N 2
#SBATCH -n 4

#SBATCH --job-name=reduction_ROME
#SBATCH -o outROME.txt

spack load hpx
mpirun hostname
mpirun ./../build/reduction --hpx:ignore-batch-env --hpx:threads 2
