@echo off
setlocal

REM Build and run flpenum with vendored yaml-cpp
set INC_DIR=third_party\yaml-cpp-yaml-cpp-0.7.0\include
set LIB_DIR=third_party\yaml-cpp-yaml-cpp-0.7.0\build

if command -v mpic++ >/dev/null 2>&1; then
  echo "Building with mpic++..."
  mpic++ -O2 -fopenmp main.cpp flpenum/flpenum.cpp config/config.cpp \
    -I. -Iconfig -Iflpenum -I"${INC_DIR}" \
    -L"${LIB_DIR}" -lyaml-cpp -o flpenum
else
  echo "mpic++ not found; building with g++ (no MPI) ..."
  g++ -O2 -fopenmp main.cpp flpenum/flpenum.cpp config/config.cpp \
    -I. -Iconfig -Iflpenum -I"${INC_DIR}" \
    -L"${LIB_DIR}" -lyaml-cpp -o flpenum
fi

if errorlevel 1 (
  echo Build failed.
  exit /b 1
)
else (
  echo Build succeeded.
)

echo Running...
.\main.exe %*
echo Run complete.

endlocal
