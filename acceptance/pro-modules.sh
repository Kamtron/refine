
esp_module=ESP/121

module load intel_2018.3.222
module load mpt-2.23

export mpi_path="/opt/hpe/hpc/mpt/mpt-2.23"

export module_path="/u/shared/fun3d/fun3d_users/modules"

export parmetis32_path="${module_path}/ParMETIS/4.0.3-mpt-2.23-intel_2018.3.222"
export parmetis64_path="${module_path}/ParMETIS-64/4.0.3-mpt-2.23-intel_2018.3.222"
export zoltan_path="${module_path}/Zoltan/3.82-mpt-2.23-intel_2018.3.222"

export egads_path="${module_path}/${esp_module}/EngSketchPad"
export opencascade_path="${module_path}/${esp_module}/OpenCASCADE"

