#!/usr/bin/env bash

#SBATCH -p rome
#SBATCH -w ziti-rome0,ziti-rome2
#SBATCH --exclusive

#SBATCH --job-name=my_hpx_program
#SBATCH -o test.txt

srun ../build/my_hpx_program --hpx:threads 8
