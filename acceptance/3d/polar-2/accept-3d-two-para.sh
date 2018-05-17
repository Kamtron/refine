#!/usr/bin/env bash

set -x # echo commands
set -e # exit on first error
set -u # Treat unset variables as error

if [ $# -gt 0 ] ; then
    one=$1/one
    two=$1/src
else
    one=${HOME}/refine/zoltan/one
    two=${HOME}/refine/zoltan/src
fi

${two}/ref_acceptance 1 ref_adapt_test.b8.ugrid

function adapt_cycle {
    proj=$1
    field=polar-2
    
    cp ref_adapt_test.b8.ugrid ${proj}.b8.ugrid

    ${two}/ref_translate ${proj}.b8.ugrid ${proj}.html
    ${two}/ref_translate ${proj}.b8.ugrid ${proj}.tec

    ${two}/ref_acceptance -ugawg ${field} ${proj}.b8.ugrid ${proj}.metric
    
    rm ref_adapt_test.b8.ugrid
    mpiexec -np 8 ${two}/ref_driver -i ${proj}.b8.ugrid -m ${proj}.metric -t
    cp ref_driver.b8.ugrid ref_adapt_test.b8.ugrid
    cp ref_gather_movie.tec ${proj}_movie.tec
    
    ${two}/ref_metric_test ${proj}.b8.ugrid ${proj}.metric > ${proj}.status
    cp ref_metric_test_s00_n1_p0_ellipse.tec ${proj}_metric_ellipse.tec
}

adapt_cycle accept-3d-two-00
adapt_cycle accept-3d-two-01
adapt_cycle accept-3d-two-02
adapt_cycle accept-3d-two-03

cat accept-3d-two-03.status
../../check.rb accept-3d-two-03.status 0.1 2.2

