
#!/usr/bin/env bash

set -x # echo commands
set -e # exit on first error
set -u # Treat unset variables as error

if [ $# -gt 0 ] ; then
    src=$1/src
else
    src=${HOME}/refine/egads/src
fi

tecplot=-t
field="-u lisbon"
egads="-g lisbon.egads"

function adapt_cycle {
    inproj=$1
    outproj=$2
    complexity=$3

    ${src}/ref_acceptance ${field} ${inproj}.meshb \
	  ${inproj}.solb
    ${src}/ref visualize ${inproj}.meshb \
	  ${inproj}.solb ${inproj}.plt

    ${src}/ref multiscale ${inproj}.meshb ${inproj}.solb \
	  ${complexity} ${inproj}-metric.solb | tee ${inproj}-multi.txt

    mpiexec ${src}/refmpi adapt ${inproj}.meshb ${egads} \
	    -m ${inproj}-metric.solb \
	  -x ${outproj}.meshb -f ${outproj}.tec > ${inproj}-adapt.txt
    tail -50 ${inproj}-adapt.txt

    ${src}/ref_acceptance ${field} ${outproj}.meshb \
	  ${outproj}.solb
    ${src}/ref visualize ${outproj}.meshb \
	  ${outproj}.solb ${outproj}.plt
}

serveCSM -batch lisbon.csm
ref boot lisbon.egads

adapt_cycle lisbon-vol lisbon01 1000
adapt_cycle lisbon01 lisbon02 2000
adapt_cycle lisbon02 lisbon03 4000
adapt_cycle lisbon03 lisbon04 8000
adapt_cycle lisbon04 lisbon05 16000
adapt_cycle lisbon05 lisbon06 32000
adapt_cycle lisbon06 lisbon07 64000
adapt_cycle lisbon07 lisbon08 128000
adapt_cycle lisbon08 lisbon09 256000
adapt_cycle lisbon09 lisbon10 512000

