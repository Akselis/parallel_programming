#!/usr/bin/env bash
set -euo pipefail

CONFIG_PATH="${1:-config.yaml}"

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
mpic++ -O2 -fopenmp main.cpp flpenum/flpenum.cpp config/config.cpp \
  -I. -Iconfig -Iflpenum -I"${INC_DIR}" \
  -L"${LIB_DIR}" -lyaml-cpp -o flpenum_app

if command -v yq >/dev/null 2>&1; then
  echo "yq found; launching one mpirun per runconfig using threads as ranks"
  run_count=$(yq '.runconfig | length' "${CONFIG_PATH}")
  if [ "${run_count}" -eq 0 ]; then
    echo "No runconfig entries found in ${CONFIG_PATH}"
    exit 1
  fi
  for idx in $(seq 0 $((run_count-1))); do
    ranks=$(yq ".runconfig[${idx}].threads // 1" "${CONFIG_PATH}")
    echo "Run $((idx+1))/${run_count}: mpirun -np ${ranks} ./flpenum_app ${CONFIG_PATH}"
    mpirun -np "${ranks}" ./flpenum_app "${CONFIG_PATH}"
  done
else
  echo "yq not found; running a single mpirun with default ranks (use -np yourself)"
  mpirun ./flpenum_app "${CONFIG_PATH}"
fi
