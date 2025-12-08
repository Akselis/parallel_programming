#!/usr/bin/env bash
set -euo pipefail

INC_DIR="third_party/yaml-cpp-yaml-cpp-0.7.0/include"
LIB_DIR="third_party/yaml-cpp-yaml-cpp-0.7.0/build_mpi"

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

# Run once; set ranks with MPI_NP or via MPIRUN_FLAGS.
MPIRUN=${MPIRUN:-mpirun}
NP_ARG=${MPI_NP:+"-np ${MPI_NP}"}
echo "Running: ${MPIRUN} ${NP_ARG} ${MPIRUN_FLAGS:-} ./flpenum_mpi_app ${*}"
${MPIRUN} ${NP_ARG} ${MPIRUN_FLAGS:-} ./flpenum_mpi_app "$@"
echo "Done."
