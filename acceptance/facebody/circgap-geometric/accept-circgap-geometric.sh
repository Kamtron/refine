
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
field="-u circgap"
egads="-g circgap.egads"

cp circgap.meshb cycle00.meshb

function adapt_cycle {
    inproj=$1
    outproj=$2

    ${src}/ref_acceptance ${field} ${inproj}.meshb \
	  ${inproj}.solb

    ${src}/ref distance ${inproj}.meshb \
	  ${inproj}-distance.solb \
	  --viscous-tags 2,5

    ${src}/ref_phys_test --geometric \
	  ${inproj}.meshb \
          ${inproj}-distance.solb \
          0.01 1.2 \
	  ${inproj}-scalar.solb

    ${src}/ref multiscale ${inproj}.meshb ${inproj}-scalar.solb \
	  1000 ${inproj}-metric.solb | tee ${inproj}-multi.txt

    ${src}/ref adapt ${inproj}.meshb ${egads} -m ${inproj}-metric.solb \
	  -x ${outproj}.meshb -f ${outproj}.tec > ${inproj}-adapt.txt
    tail -50 ${inproj}-adapt.txt

    ${src}/ref_acceptance ${field} ${outproj}.meshb \
	  ${outproj}.solb
}

adapt_cycle cycle00 cycle01
adapt_cycle cycle01 cycle02
adapt_cycle cycle02 cycle03
adapt_cycle cycle03 cycle04
adapt_cycle cycle04 cycle05
adapt_cycle cycle05 cycle06
adapt_cycle cycle06 cycle07
adapt_cycle cycle07 cycle08
adapt_cycle cycle08 cycle09
adapt_cycle cycle09 cycle10

# mpiexec refmpi viz cycle09.meshb cycle09.solb cycle09-iso.meshb --iso 0 20.0
# ref tr cycle09-iso.meshb cycle09-iso.tec

# mpiexec refmpi viz cycle09.meshb cycle09.solb cycle09-iso.tec --iso 0 20.0

