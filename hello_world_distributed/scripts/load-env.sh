#!/bin/bash

echo "Setting up environment variables"
spack load gcc@11.3.0
spack load benchmark@1.7.1
spack load hpx@1.9.0
##spack load openmpi@4.1.3
