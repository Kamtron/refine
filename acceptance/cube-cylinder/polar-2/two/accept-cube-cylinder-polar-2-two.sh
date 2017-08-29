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

field=polar-2

function adapt_cycle_sant {
    inproj=$1
    outproj=$2
    sweeps=$3

    ${two}/ref_driver -i ${inproj}.meshb -g ega.egads -m ${inproj}.metric -o ${outproj} -s ${sweeps} -l
    ${two}/ref_acceptance -ugawg ${field} ${outproj}.meshb ${outproj}.metric
    ${two}/ref_metric_test ${outproj}.meshb ${outproj}.metric > ${outproj}.status

}
function adapt_cycle {
    inproj=$1
    outproj=$2
    sweeps=$3

    ${two}/ref_driver -i ${inproj}.meshb -g ega.egads -m ${inproj}.metric -o ${outproj} -s ${sweeps}
    ${two}/ref_acceptance -ugawg ${field} ${outproj}.meshb ${outproj}.metric
    ${two}/ref_metric_test ${outproj}.meshb ${outproj}.metric > ${outproj}.status

}

# ${two}/ref_geom_test ega.egads
# ${two}/ref_geom_test ega.egads ega.ugrid

${two}/ref_acceptance -ugawg ${field} ega.meshb ega.metric

adapt_cycle_sant ega cycle01 2
adapt_cycle_sant cycle01 cycle02 2
adapt_cycle_sant cycle02 cycle03 2
adapt_cycle_sant cycle03 cycle04 2
adapt_cycle_sant cycle04 cycle05 2
adapt_cycle_sant cycle05 cycle06 2
adapt_cycle cycle06 cycle07 2
adapt_cycle cycle07 cycle08 2
adapt_cycle cycle08 cycle09 2
adapt_cycle cycle09 cycle10 2
adapt_cycle cycle10 cycle11 2

cat cycle10.status
../../../check.rb cycle10.status 0.05 2.2

exit

adapt_cycle cycle11 cycle12 2
adapt_cycle cycle12 cycle13 2
adapt_cycle cycle13 cycle14 2
adapt_cycle cycle14 cycle15 2
adapt_cycle cycle15 cycle16 2
adapt_cycle cycle16 cycle17 2
adapt_cycle cycle17 cycle18 2
adapt_cycle cycle18 cycle19 2
adapt_cycle cycle19 cycle20 2
adapt_cycle cycle20 cycle21 2
adapt_cycle cycle21 cycle22 2
adapt_cycle cycle22 cycle23 2
adapt_cycle cycle23 cycle24 2
adapt_cycle cycle24 cycle25 2
adapt_cycle cycle25 cycle26 2
adapt_cycle cycle26 cycle27 2
adapt_cycle cycle27 cycle28 2
adapt_cycle cycle28 cycle29 2
adapt_cycle cycle29 cycle30 2

