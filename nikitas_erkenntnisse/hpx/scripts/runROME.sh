#!/usr/bin/env bash

#SBATCH -p rome
#SBATCH -w ziti-rome0
#SBATCH -N 1
#SBATCH -n 2

#SBATCH --job-name=reduction_ROME
#SBATCH -o outROME.txt

spack load hpx
mpirun hostname
mpirun ./../build/reduction --hpx:ignore-batch-env
