#!/usr/bin/env bash

#SBATCH -p rome
#SBATCH -w ziti-rome0,ziti-rome1
#SBATCH --exclusive
#SBATCH -N 2
#SBATCH -n 2

#SBATCH --job-name=my_hpx_program
#SBATCH -o test.txt


salloc mpirun -n 2 ../build/my_hpx_program --hpx:threads 4
