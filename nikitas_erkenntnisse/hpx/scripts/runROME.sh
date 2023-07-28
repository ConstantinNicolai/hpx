#!/usr/bin/env bash

#SBATCH -p rome
#SBATCH -w ziti-rome2,ziti-rome3
#SBATCH -N 2
#SBATCH -n 4

#SBATCH --job-name=my_hpx_program_ROME
#SBATCH -o outROME.txt

spack load hpx
mpirun hostname
mpirun ./../build/my_hpx_program --hpx:ignore-batch-env --hpx:threads 2
