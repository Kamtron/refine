
esp_module=ESP/120

module use --append /u/shared/fun3d/fun3d_users/modulefiles
module use --append /u/shared/wtjones1/Modules/modulefiles

export compile_modules="gcc_6.2.0"
export compiler_rpath="/usr/local/pkgs-modules/intel_2017.0.098/compilers_and_libraries_2017.2.174/linux/compiler/lib/intel64_lin"

# expected to move intel to compile mods, but not working with openmpi
export run_modules="intel_2017.2.174 openmpi_2.1.1_intel_2017 tetgen ${esp_module} GEOLAB/geolab_64 GEOLAB/AFLR3-16.28.5 valgrind_3.13.0"

module load ${compile_modules} ${run_modules}

export module_path="/u/shared/fun3d/fun3d_users/modules"

export mpi_path="/usr/local/pkgs-modules/openmpi_2.1.1_intel_2017"

export parmetis_path="${module_path}/ParMETIS/4.0.3-openmpi-2.1.1-intel_2017.2.174"
export zoltan_path="${module_path}/Zoltan/3.82-openmpi-1.10.7-intel_2017.2.174"

export egads_path="${module_path}/${esp_module}/EngSketchPad"
export opencascade_path="${module_path}/${esp_module}/OpenCASCADE"
