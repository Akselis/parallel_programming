#pragma once
#include <string>
#include <vector>
#include "config/config.h"

// Shared data (definitions in flpenum.cpp)
extern int numDP, numPF, numCL, numX;
extern std::string demandFile;
extern double **demandPoints;
extern double **distanceMatrix;
extern int *X;
extern int *bestX;
extern double u, bestU;

struct RunResult {
    int type = 0;
    int threads = 1;
    int numDP = 0;
    int numPF = 0;
    int numCL = 0;
    int numX = 0;
    double matrixTime = 0.0;
    double searchTime = 0.0;
    double totalTime = 0.0;
    double bestUtility = 0.0;
    std::vector<int> bestSolution;
};

// Functions implemented in flpenum.cpp
double getTime();
void loadDemandPoints();
double HaversineDistance(double lat1, double lon1, double lat2, double lon2);
double HaversineDistance(int i, int j);
double evaluateSolution(const int* X);
int increaseX(int* X, int index, int maxindex);

// Entry point you'll call from main.cpp
RunResult runEnumeration(const RunConfig& config);

// MPI-enabled enumeration (defined in flpenum_mpi.cpp)
RunResult runEnumerationMPI(const RunConfig& config);
