#!/usr/bin/env bash

#SBATCH -p qdr
#SBATCH -w ziti-qdr009,ziti-qdr010
#SBATCH -N 2
#SBATCH -n 4

#SBATCH --job-name=reduction_QDR
#SBATCH -o outQDR.txt

spack load hpx
mpirun hostname
mpirun ./../build/reduction --hpx:threads 2
