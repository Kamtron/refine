#!/usr/bin/env bash

set -x # echo commands
set -e # exit on first error
set -u # Treat unset variables as error

if [ $# -gt 0 ] ; then
    one=$1/src
    two=$1/two
else
    one=${HOME}/refine/egads/src
    two=${HOME}/refine/egads/two
fi

geomfile=ega.egads

# ${two}/ref_geom_test ${geomfile}
# ${two}/ref_geom_test ${geomfile} ega.meshb

${two}/ref_acceptance ega.meshb ega.metric 0.1
${two}/ref_driver -i ega.meshb -g ${geomfile} -m ega.metric -o ref_driver1
${two}/ref_acceptance ref_driver1.meshb ref_driver1.metric 0.1
${two}/ref_metric_test ref_driver1.meshb ref_driver1.metric > accept-cube-cylinder-uniform-two-01.status

${two}/ref_driver -i ref_driver1.meshb -g ${geomfile} -m ref_driver1.metric -o ref_driver2
${two}/ref_acceptance ref_driver2.meshb ref_driver2.metric 0.1
${two}/ref_metric_test ref_driver2.meshb ref_driver2.metric > accept-cube-cylinder-uniform-two-02.status

cat accept-cube-cylinder-uniform-two-02.status
../../../check.rb accept-cube-cylinder-uniform-two-02.status 0.20 1.6



