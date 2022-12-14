#!/usr/bin/env bash

set -x # echo commands
set -e # exit on first error
set -u # Treat unset variables as error

if [ $# -gt 0 ] ; then
    one=$1/one
    two=$1/src
else
    one=${HOME}/refine/egads/one
    two=${HOME}/refine/egads/src
fi

geomfile=blister.egads
rm -f ${geomfile}

serveCSM -batch blister.csm

rm -f blister-vol.meshb
${two}/ref boostrap ${geomfile}

rm -f blister01.meshb
${two}/ref adapt blister-vol.meshb -g ${geomfile} -x blister01.meshb -t \
      -f blister01-final.tec
mv ref_gather_movie.tec blister01-movie.tec

${two}/ref_histogram_test blister01.meshb blister01-final-metric.solb \
 > accept-blister-curvature.status

cat accept-blister-curvature.status
../../check.rb accept-blister-curvature.status 0.2 2.2



