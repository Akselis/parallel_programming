#!/bin/bash
#SBATCH -p main
#SBATCH -n4
module load #%Moduleopenmpi
set -euo pipefail

INC_DIR="third_party/yaml-cpp-yaml-cpp-0.7.0/include"
LIB_DIR="third_party/yaml-cpp-yaml-cpp-0.7.0/build_mpi"

# Clean stale CMake cache if the source path moved
if [ -f "${LIB_DIR}/CMakeCache.txt" ]; then
  rm -rf "${LIB_DIR}"
fi

echo "Configuring yaml-cpp with mpic++..."
cmake -S third_party/yaml-cpp-yaml-cpp-0.7.0 -B "${LIB_DIR}" \
  -G "Unix Makefiles" \
  -DCMAKE_CXX_COMPILER=mpic++ \
  -DYAML_BUILD_SHARED_LIBS=OFF \
  -DYAML_CPP_BUILD_TESTS=OFF \
  -DYAML_CPP_BUILD_TOOLS=OFF

echo "Building yaml-cpp..."
cmake --build "${LIB_DIR}" --config Release -- -j"$(nproc)"

echo "Building application with mpic++..."
mpic++ -O2 -fopenmp main_mpi.cpp flpenum/flpenum.cpp flpenum/flpenum_mpi.cpp config/config.cpp \
  -I. -Iconfig -Iflpenum -I"${INC_DIR}" \
  -L"${LIB_DIR}" -lyaml-cpp -o flpenum_mpi_app

MPIRUN=${MPIRUN:-mpirun}

if command -v ./yq >/dev/null 2>&1; then
  CONFIG_PATH=${1:-./yaml/config_hpc.yaml}
  echo "yq found; iterating runconfig entries in ${CONFIG_PATH}"
  run_count=$(./yq '.runconfig | length' "${CONFIG_PATH}")
  if [ "${run_count}" -eq 0 ]; then
    echo "No runconfig entries found in ${CONFIG_PATH}"
    exit 1
  fi
  for idx in $(seq 0 $((run_count-1))); do
    ranks=$(./yq ".runconfig[${idx}].threads // 1" "${CONFIG_PATH}")
    echo "Run $((idx+1))/${run_count}: mpirun -np ${ranks} ${MPIRUN_FLAGS:-} ./flpenum_mpi_app ${CONFIG_PATH} ${idx}"
    ${MPIRUN} -np "${ranks}" ${MPIRUN_FLAGS:-} ./flpenum_mpi_app "${CONFIG_PATH}" "${idx}"
  done
else
  echo "yq not found; running a single mpirun with default ranks (use -np yourself)"
  NP_ARG=${MPI_NP:+"-np ${MPI_NP}"}
  echo "Running: ${MPIRUN} ${NP_ARG} ${MPIRUN_FLAGS:-} ./flpenum_mpi_app $*"
  ${MPIRUN} ${NP_ARG} ${MPIRUN_FLAGS:-} ./flpenum_mpi_app "$@"
fi
echo "Done."
