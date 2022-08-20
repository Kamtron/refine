#!/usr/bin/env bash

set -x # echo commands
set -e # exit on first error
set -u # Treat unset variables as error

if [ $# -gt 0 ] ; then
    src=$1/src
else
    src=${HOME}/refine/egads/src
fi

serveCSM -batch poly.csm 

${src}/ref bootstrap poly.egads

${src}/ref collar normal \
    poly-vol.meshb \
    10 \
    0.1 \
    2.0 \
    1.68 \
    --fun3d-mapbc poly-vol.mapbc \
    -x poly-inf.meshb

exit
