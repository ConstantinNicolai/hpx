#!/usr/bin/env bash

#SBATCH -p rome
#SBATCH -w ziti-rome0
#SBATCH -N 1

#SBATCH --job-name=transform_ROME
#SBATCH -o outROME_transfrom.txt

REPS=1

spack load hpx
mpirun hostname

mpirun ./../build/transform   --benchmark_time_unit=us --benchmark_out_format=csv --benchmark_out=transform_rome.csv \
                              --benchmark_report_aggregates_only=true --benchmark_repetitions=$REPS \
                              --benchmark_min_time=0.001 --benchmark_min_warmup_time=0.01
