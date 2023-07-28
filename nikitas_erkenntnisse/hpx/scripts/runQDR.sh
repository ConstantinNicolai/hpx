#!/usr/bin/env bash

#SBATCH -p qdr
#SBATCH -w ziti-qdr009,ziti-qdr010
#SBATCH -N 2
#SBATCH -n 4

#SBATCH --job-name=my_hpx_program_QDR
#SBATCH -o outQDR.txt

spack load hpx
mpirun hostname
mpirun ./../build/my_hpx_program --hpx:ignore-batch-env --hpx:threads 2
