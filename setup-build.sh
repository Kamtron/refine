#!/usr/bin/env bash

set -x

# TESTS_ENVIRONMENT='valgrind --quiet --leak-check=full'

mkdir -p strict
( cd strict && \
    ../configure \
    --prefix=`pwd` \
    CFLAGS='-g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused -Wuninitialized' \
    FC=gfortran \
    ) \
    || exit

mkdir -p mpi
( cd mpi && \
    ../configure \
    --prefix=`pwd` \
    --with-zoltan=${HOME}/local/pkgs/zoltan_distrib_v3.6/Zoltan_v3.6/build \
    CC=mpicc \
    FC=mpif90 \
    CFLAGS='-DHAVE_MPI -g -O2 -traceback -Wall -Wcheck -ftrapuv -fp-stack-check -check-uninit -fstack-security-check' \
    ) \
    || exit

mkdir -p profile
( cd profile && \
    ../configure \
    --prefix=`pwd` \
    CFLAGS='-g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused -pg' \
    FC=gfortran \
    ) \
    || exit

