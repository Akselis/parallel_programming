#!/usr/bin/env bash
set -euo pipefail

INC_DIR="third_party/yaml-cpp-yaml-cpp-0.7.0/include"
LIB_DIR="third_party/yaml-cpp-yaml-cpp-0.7.0/build"

echo "Building with mpic++..."
mpic++ -O2 -fopenmp main.cpp flpenum/flpenum.cpp config/config.cpp \
  -I. -Iconfig -Iflpenum -I"${INC_DIR}" \
  -L"${LIB_DIR}" -lyaml-cpp -o flpenum_app

echo "Running..."
./flpenum_app "$@"
