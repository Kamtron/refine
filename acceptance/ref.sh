#!/usr/bin/env bash

set -e # exit on first error
set -u # Treat unset variables as error

# Setup bash module environment
set +x # echo commands off for module
source /usr/local/pkgs/modules/init/bash
module purge
source acceptance/ref-modules.sh
module list
set -x # echo commands

root_dir=$(dirname $PWD)
source_dir=${root_dir}/refine

parmetis_dir=${root_dir}/_refine-parmetis-egadslite
zoltan_dir=${root_dir}/_refine-zoltan-egadslite
egads_dir=${root_dir}/_refine-egads-full
strict_dir=${root_dir}/_refine-strict
asan_dir=${root_dir}/_refine-asan
cpp_dir=${root_dir}/_refine-cpp

cd ${source_dir}
LOG=${root_dir}/log.bootstrap
trap "cat $LOG" EXIT
./bootstrap > $LOG 2>&1
trap - EXIT

date

mkdir -p ${asan_dir}
cd ${asan_dir}

LOG=${root_dir}/log.asan-configure
trap "cat $LOG" EXIT
${source_dir}/configure \
    --prefix=${asan_dir} \
    CFLAGS='-g -O1 -fsanitize=address -fno-omit-frame-pointer -pedantic-errors -Wall -Wextra -Werror -Wunused -Wuninitialized' \
    CC=gcc  > $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.asan-make-check
trap "cat $LOG" EXIT
  make -j 8 > $LOG 2>&1
  make check > $LOG 2>&1
trap - EXIT

date

mkdir -p ${strict_dir}
cd ${strict_dir}

LOG=${root_dir}/log.strict-configure
trap "cat $LOG" EXIT
${source_dir}/configure \
    --prefix=${strict_dir} \
    CFLAGS='-g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused -Wuninitialized' \
    CC=gcc  > $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.strict-make-check-valgrind
trap "cat $LOG" EXIT
make -j 8 > $LOG 2>&1
make check TESTS_ENVIRONMENT='valgrind --quiet  --error-exitcode=1 --leak-check=full' >> $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.strict-make-distcheck
trap "cat $LOG" EXIT
( make distcheck > $LOG 2>&1 && cp refine-*.tar.gz ${root_dir} || touch FAILED ) &
trap - EXIT

date

LOG=${root_dir}/log.cpp-configure-compile-check
trap "cat $LOG" EXIT
${source_dir}/configure \
    --prefix=${strict_dir} \
    CFLAGS='-g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused -Wuninitialized' \
    CC=g++  > $LOG 2>&1
make -j 8 >> $LOG 2>&1
make check  >> $LOG 2>&1
trap - EXIT

date

mkdir -p ${egads_dir}
cd ${egads_dir}

LOG=${root_dir}/log.egads-configure
trap "cat $LOG" EXIT
${source_dir}/configure \
    --prefix=${egads_dir} \
    --with-mpi=${mpi_path} \
    --with-parmetis=${parmetis_path} \
    --with-EGADS=${egads_path} \
    --with-OpenCASCADE=${opencascade_path} \
    CFLAGS='-g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused -Wuninitialized' \
    --with-compiler-rpath=${compiler_rpath} \
    CC=icc  > $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.egads-make
trap "cat $LOG" EXIT
env TMPDIR=${PWD} make -j 8  > $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.egads-make-install
trap "cat $LOG" EXIT
make install > $LOG 2>&1
trap - EXIT

date

mkdir -p ${zoltan_dir}
cd ${zoltan_dir}

LOG=${root_dir}/log.zoltan-configure
trap "cat $LOG" EXIT
which mpicc
which mpicc >> $LOG 2>&1
${source_dir}/configure \
    --prefix=${zoltan_dir} \
    --with-zoltan=${zoltan_path} \
    --with-EGADS=${egads_path} \
    --enable-lite \
    --with-compiler-rpath=${compiler_rpath} \
    CFLAGS='-DHAVE_MPI -g -O2 -traceback -Wall -ftrapuv' \
    CC=mpicc > $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.zoltan-make
trap "cat $LOG" EXIT
env TMPDIR=${PWD} make -j 8  > $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.zoltan-make-install
trap "cat $LOG" EXIT
make install > $LOG 2>&1
trap - EXIT

date

mkdir -p ${parmetis_dir}
cd ${parmetis_dir}

LOG=${root_dir}/log.parmetis-configure
trap "cat $LOG" EXIT
which mpicc
which mpicc >> $LOG 2>&1
${source_dir}/configure \
    --prefix=${parmetis_dir} \
    --with-parmetis=${parmetis_path} \
    --with-EGADS=${egads_path} \
    --enable-lite \
    --with-compiler-rpath=${compiler_rpath} \
    CFLAGS='-DHAVE_MPI -g -O2 -traceback -Wall -ftrapuv -fp-stack-check -fstack-protector-all -fstack-security-check' \
    CC=mpicc > $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.parmetis-make
trap "cat $LOG" EXIT
env TMPDIR=${PWD} make -j 8  > $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.parmetis-make-install
trap "cat $LOG" EXIT
make install > $LOG 2>&1
trap - EXIT

date

module purge
module load ${run_modules}

LOG=${root_dir}/log.zoltan-unit
trap "cat $LOG" EXIT
cd ${zoltan_dir}/src
echo para-unit > $LOG 2>&1
which mpiexec
which mpiexec >> $LOG 2>&1
mpiexec -np 2 ./ref_agents_test >> $LOG 2>&1
mpiexec -np 2 ./ref_dict_test >> $LOG 2>&1
mpiexec -np 2 ./ref_edge_test >> $LOG 2>&1
mpiexec -np 2 ./ref_gather_test >> $LOG 2>&1
mpiexec -np 2 ./ref_grid_test >> $LOG 2>&1
mpiexec -np 2 ./ref_interp_test >> $LOG 2>&1
mpiexec -np 2 ./ref_iso_test >> $LOG 2>&1
mpiexec -np 2 ./ref_metric_test >> $LOG 2>&1
mpiexec -np 2 ./ref_mpi_test >> $LOG 2>&1
mpiexec -np 2 ./ref_node_test >> $LOG 2>&1
mpiexec -np 2 ./ref_part_test >> $LOG 2>&1
mpiexec -np 2 ./ref_phys_test >> $LOG 2>&1
mpiexec -np 2 ./ref_migrate_test >> $LOG 2>&1
mpiexec -np 2 ./ref_cavity_test >> $LOG 2>&1
mpiexec -np 2 ./ref_elast_test >> $LOG 2>&1
mpiexec -np 2 ./ref_recon_test >> $LOG 2>&1
mpiexec -np 2 ./ref_search_test >> $LOG 2>&1
mpiexec -np 2 ./ref_shard_test >> $LOG 2>&1
mpiexec -np 2 ./ref_subdiv_test >> $LOG 2>&1
mpiexec -np 8 ./ref_agents_test >> $LOG 2>&1
mpiexec -np 8 ./ref_dict_test >> $LOG 2>&1
mpiexec -np 8 ./ref_edge_test >> $LOG 2>&1
mpiexec -np 8 ./ref_gather_test >> $LOG 2>&1
mpiexec -np 8 ./ref_grid_test >> $LOG 2>&1
mpiexec -np 8 ./ref_interp_test >> $LOG 2>&1
mpiexec -np 8 ./ref_iso_test >> $LOG 2>&1
mpiexec -np 8 ./ref_metric_test >> $LOG 2>&1
mpiexec -np 8 ./ref_mpi_test >> $LOG 2>&1
mpiexec -np 8 ./ref_node_test >> $LOG 2>&1
mpiexec -np 8 ./ref_part_test >> $LOG 2>&1
mpiexec -np 8 ./ref_phys_test >> $LOG 2>&1
mpiexec -np 8 ./ref_migrate_test >> $LOG 2>&1
mpiexec -np 8 ./ref_cavity_test >> $LOG 2>&1
mpiexec -np 8 ./ref_elast_test >> $LOG 2>&1
mpiexec -np 8 ./ref_recon_test >> $LOG 2>&1
mpiexec -np 8 ./ref_search_test >> $LOG 2>&1
mpiexec -np 8 ./ref_shard_test >> $LOG 2>&1
mpiexec -np 8 ./ref_subdiv_test >> $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.parmetis-unit
trap "cat $LOG" EXIT
cd ${parmetis_dir}/src
echo para-unit > $LOG 2>&1
which mpiexec
which mpiexec >> $LOG 2>&1
mpiexec -np 2 ./ref_agents_test >> $LOG 2>&1
mpiexec -np 2 ./ref_edge_test >> $LOG 2>&1
mpiexec -np 2 ./ref_gather_test >> $LOG 2>&1
mpiexec -np 2 ./ref_grid_test >> $LOG 2>&1
mpiexec -np 2 ./ref_interp_test >> $LOG 2>&1
mpiexec -np 2 ./ref_iso_test >> $LOG 2>&1
mpiexec -np 2 ./ref_metric_test >> $LOG 2>&1
mpiexec -np 2 ./ref_mpi_test >> $LOG 2>&1
mpiexec -np 2 ./ref_node_test >> $LOG 2>&1
mpiexec -np 2 ./ref_part_test >> $LOG 2>&1
mpiexec -np 2 ./ref_phys_test >> $LOG 2>&1
mpiexec -np 2 ./ref_migrate_test >> $LOG 2>&1
mpiexec -np 2 ./ref_cavity_test >> $LOG 2>&1
mpiexec -np 2 ./ref_elast_test >> $LOG 2>&1
mpiexec -np 2 ./ref_recon_test >> $LOG 2>&1
mpiexec -np 2 ./ref_search_test >> $LOG 2>&1
mpiexec -np 2 ./ref_shard_test >> $LOG 2>&1
mpiexec -np 2 ./ref_subdiv_test >> $LOG 2>&1
mpiexec -np 8 ./ref_agents_test >> $LOG 2>&1
mpiexec -np 8 ./ref_edge_test >> $LOG 2>&1
mpiexec -np 8 ./ref_gather_test >> $LOG 2>&1
mpiexec -np 8 ./ref_grid_test >> $LOG 2>&1
mpiexec -np 8 ./ref_interp_test >> $LOG 2>&1
mpiexec -np 8 ./ref_iso_test >> $LOG 2>&1
mpiexec -np 8 ./ref_metric_test >> $LOG 2>&1
mpiexec -np 8 ./ref_mpi_test >> $LOG 2>&1
mpiexec -np 8 ./ref_node_test >> $LOG 2>&1
mpiexec -np 8 ./ref_part_test >> $LOG 2>&1
mpiexec -np 8 ./ref_phys_test >> $LOG 2>&1
mpiexec -np 8 ./ref_migrate_test >> $LOG 2>&1
mpiexec -np 8 ./ref_cavity_test >> $LOG 2>&1
mpiexec -np 8 ./ref_elast_test >> $LOG 2>&1
mpiexec -np 8 ./ref_recon_test >> $LOG 2>&1
mpiexec -np 8 ./ref_search_test >> $LOG 2>&1
mpiexec -np 8 ./ref_shard_test >> $LOG 2>&1
mpiexec -np 8 ./ref_subdiv_test >> $LOG 2>&1
trap - EXIT

LOG=${root_dir}/log.accept-2d-linear
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/linear
( ./accept-2d-linear.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-2d-polar-2
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/polar-2
( ./accept-2d-polar-2.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-2d-masabl
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/masabl
( ./accept-2d-masabl.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-2d-mixed
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/mixed
( ./accept-2d-mixed.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-2d-circle
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/circle
( ./accept-2d-circle.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-2d-yplus
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/yplus
( ./accept-2d-yplus.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-triangle
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/triangle
( ./accept-facebody-iso01.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-side
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/side
( ./accept-facebody-side.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-polar-2
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/polar-2
( ./accept-facebody-polar-2.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-init-visc
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/init-visc
( ./accept-facebody-init-visc.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-parabola
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/parabola
( ./accept-facebody-parabola.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

sleep 10 # allow some tests to complete before making more

LOG=${root_dir}/log.accept-facebody-trig-multi
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/trig-multi
( ./accept-facebody-trig-multi.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-trig-fixed-point
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/trig-fixed-point
( ./accept-facebody-trig-fixed-point.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-trig-opt-goal
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/trig-opt-goal
( ./accept-facebody-trig-opt-goal.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-trig-cons-euler
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/trig-cons-euler
( ./accept-facebody-trig-cons-euler.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-trig-cons-visc
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/trig-cons-visc
( ./accept-facebody-trig-cons-visc.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-vortex-multi
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/vortex-multi
( ./accept-facebody-vortex-multi.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-vortex-opt-goal
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/vortex-opt-goal
( ./accept-facebody-vortex-opt-goal.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

sLOG=${root_dir}/log.accept-facebody-ringleb-multi
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/ringleb-multi
( ./accept-facebody-ringleb-multi.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-ringleb-opt-goal
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/ringleb-opt-goal
( ./accept-facebody-ringleb-opt-goal.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-ringleb-solb-interp
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/ringleb-solb-interp
( ./accept-facebody-ringleb-solb-interp.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-facebody-combine
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/combine
( ./accept-facebody-combine.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

sleep 10 # allow some tests to complete before making more

LOG=${root_dir}/log.accept-3d-linear
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/3d/linear
( ./accept-3d-linear.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cube-initial-cell
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube/initial-cell
( ./accept-cube-initial-cell.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cube-cylinder-gen-aflr3
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-cylinder/gen/aflr3
( ./accept-cube-cylinder-aflr3.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cube-cylinder-uniform
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-cylinder/uniform
( ./accept-cube-cylinder-uniform.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cube-cylinder-linear010
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-cylinder/linear010
( ./accept-cube-cylinder-linear010.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cube-cylinder-polar-2
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-cylinder/polar-2
( ./accept-cube-cylinder-polar-2.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

sleep 10 # allow some tests to complete before making more

LOG=${root_dir}/log.accept-cube-sphere-uniform-nogeom
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-sphere/uniform
( ./accept-cube-sphere-uniform-nogeom.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cube-sphere-uniform
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-sphere/uniform
( ./accept-cube-sphere-uniform.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cube-sphere-ring
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-sphere/ring
( ./accept-cube-sphere-ring.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cube-wire-coarse
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-wire/coarse
( ./accept-cube-wire-coarse.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-bite-coarse
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/bite/coarse
( ./accept-bite-coarse.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-annulus-uniform
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/annulus/uniform
( ./accept-annulus-uniform.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-annulus-coarse
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/annulus/coarse
( ./accept-annulus-coarse.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cone-cone-uniform
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cone-cone/uniform
( ./accept-cone-cone-uniform.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-cone-cone-recon
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cone-cone/recon
( ./accept-cone-cone-recon.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-sliver-bootstrap
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/sliver/bootstrap
( ./accept-sliver.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-offset-cylcyl
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/offset/cylcyl
( ./accept-offset-cylcyl.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-offset-boxbox
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/offset/boxbox
( ./accept-offset-boxbox.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

sleep 10 # allow some tests to complete before making more

LOG=${root_dir}/log.accept-om6-recon
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/om6/recon
( ./accept-om6-recon.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-revolve-pencil-curve
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/revolve-pencil/curve
( ./revolve-pencil-curve.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-hemisphere-uniform
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/hemisphere/uniform
( ./accept-hemisphere-uniform.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-hemisphere-prism
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/hemisphere/prism
( ./accept-hemisphere-prism.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-hemisphere-normal-spacing
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/hemisphere/normal-spacing
( ./accept-hemisphere-normal-spacing.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-hemisphere-spacing-table
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/hemisphere/spacing-table
( ./accept-hemisphere-spacing-table.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-sphere-cube-tetgen
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/sphere-cube/tetgen
( ./accept-sphere-cube-init.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-3d-sinfun3
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/3d/sinfun3
( ./accept-3d-sinfun3.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

sleep 10 # allow some tests to complete before making more

LOG=${root_dir}/log.accept-inflate-collar
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/inflate/collar
( ./inflate.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-inflate-interp
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/inflate/interp
( ./interp.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-inflate-radial
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/inflate/radial
( ./inflate.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-inflate-mapbc
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/inflate/mapbc
( ./inflate.sh ${strict_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-inflate-cigar
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/inflate/cigar
( ./accept-inflate-cigar.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

LOG=${root_dir}/log.accept-narrow-cyl-spalding
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/narrow-cyl/spalding
( ./accept-narrow-cyl-spalding.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

wait

# 2 procs
LOG=${root_dir}/log.accept-2d-linear-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/linear
( ./accept-2d-linear-para.sh ${parmetis_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

# 4 procs
LOG=${root_dir}/log.accept-2d-masabl-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/masabl
( ./accept-2d-masabl-para.sh ${parmetis_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

# 4 procs
LOG=${root_dir}/log.accept-2d-polar-2-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/2d/polar-2
( ./accept-2d-polar-2-para.sh ${parmetis_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

wait

# 2 procs
LOG=${root_dir}/log.accept-facebody-side-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/facebody/side
( ./accept-facebody-side-para.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

# 2 procs
LOG=${root_dir}/log.accept-3d-linear-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/3d/linear
( ./accept-3d-linear-para.sh ${parmetis_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

# 1-1-4-4 procs
LOG=${root_dir}/log.accept-cube-cylinder-polar-2-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-cylinder/polar-2
( ./accept-cube-cylinder-polar-2-para.sh ${parmetis_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

# 4 procs
LOG=${root_dir}/log.accept-hemisphere-uniform-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/hemisphere/uniform-para
( ./accept-hemisphere-uniform-para.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

wait

# 2 procs
LOG=${root_dir}/log.accept-inflate-interp-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/inflate/interp-para
( ./interp-para.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

# 4 procs
LOG=${root_dir}/log.accept-3d-subset-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/3d/subset
( ./accept-3d-subset-para.sh ${parmetis_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

wait

# 8 procs
LOG=${root_dir}/log.accept-inflate-radial-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/inflate/radial-para
( ./inflate-para.sh ${egads_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

# 4 procs
LOG=${root_dir}/log.accept-cube-subset-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube/subset
( ./accept-cube-subset-para.sh ${parmetis_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

wait

# 8 procs
LOG=${root_dir}/log.accept-inflate-mapbc-para
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/inflate/mapbc-para
( ./inflate-para.sh ${parmetis_dir} > $LOG 2>&1 || touch FAILED ) &
trap - EXIT

wait

LOG=${root_dir}/log.accept-cube-cylinder-uniform-valgrind
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-cylinder/uniform
( ./accept-cube-cylinder-uniform-valgrind.sh ${egads_dir} > $LOG 2>&1 || touch FAILED )
trap - EXIT

LOG=${root_dir}/log.accept-cube-cylinder-uniform-valgrind-mpi
trap "cat $LOG" EXIT
cd ${source_dir}/acceptance/cube-cylinder/uniform
( ./accept-cube-cylinder-uniform-valgrind-mpi.sh ${zoltan_dir} > $LOG 2>&1 || touch FAILED )
trap - EXIT

# attempt to fix stability with grep
wait
sleep 10

grep RAC ${root_dir}/log.accept-* > ${root_dir}/log.summary

find ${root_dir} -name FAILED

echo -e \\n\
# Build has failed if any failed cases have been reported
exit `find ${root_dir} -name FAILED | wc -l`


