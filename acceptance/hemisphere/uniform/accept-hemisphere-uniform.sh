#!/usr/bin/env bash

set -x # echo commands
set -e # exit on first error
set -u # Treat unset variables as error

if [ $# -gt 0 ] ; then
    src=$1/src
else
    src=${HOME}/refine/egads/src
fi

geomfile=hemisphere.egads

serveCSM -batch -skipTess ../hemisphere.csm
${src}/ref bootstrap ${geomfile}

${src}/ref_acceptance hemisphere-vol.meshb hemisphere-vol-metric.solb 0.1
${src}/ref adapt hemisphere-vol.meshb \
      --egads ${geomfile} \
      --metric hemisphere-vol-metric.solb \
      -x hemicurve1.meshb
${src}/ref_acceptance hemicurve1.meshb hemicurve1-metric.solb 0.1
${src}/ref_metric_test hemicurve1.meshb hemicurve1-metric.solb \
      > accept-hemisphere-uniform-01.status

cat accept-hemisphere-uniform-01.status
../../check.rb accept-hemisphere-uniform-01.status 0.3 3.0

${src}/ref_acceptance -u u5 hemisphere-vol.meshb hemisphere-vol.solb
${src}/ref interpolate \
	hemisphere-vol.meshb hemisphere-vol.solb \
	hemicurve1.meshb hemicurve1-interp.solb
${src}/ref_acceptance -u u5 hemicurve1.meshb hemicurve1.solb
${src}/ref interpolate \
        hemicurve1.meshb hemicurve1.solb \
        hemisphere-vol.meshb hemisphere-vol-interp.solb

