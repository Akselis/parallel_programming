#include "flpenum.h"
#include <mpi.h>
#include <iostream>
#include <vector>
#include <omp.h>

namespace std;

// MPI-enabled enumeration (distributed memory)
RunResult runEnumerationMPI(const RunConfig& config) {
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    numDP = config.numDP;
    numPF = config.numPF;
    numCL = config.numCL;
    numX  = config.numX;
    demandFile = config.inputFile;
    omp_set_num_threads(config.threads);

    // allocate global buffers
    if (X) delete[] X;
    if (bestX) delete[] bestX;
    X = new int[numX];
    bestX = new int[numX];

    if (rank == 0) {
        loadDemandPoints();
    } else {
        demandPoints = new double*[numDP];
        for (int i = 0; i < numDP; ++i) demandPoints[i] = new double[3];
    }

    // broadcast demand points
    std::vector<double> flat(numDP * 3);
    if (rank == 0) {
        for (int i = 0; i < numDP; ++i) {
            flat[i*3 + 0] = demandPoints[i][0];
            flat[i*3 + 1] = demandPoints[i][1];
            flat[i*3 + 2] = demandPoints[i][2];
        }
    }
    MPI_Bcast(flat.data(), static_cast<int>(flat.size()), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    if (rank != 0) {
        for (int i = 0; i < numDP; ++i) {
            demandPoints[i][0] = flat[i*3 + 0];
            demandPoints[i][1] = flat[i*3 + 1];
            demandPoints[i][2] = flat[i*3 + 2];
        }
    }

    double t_start = getTime();

    // distance matrix using MPI (each rank fills a flat buffer for its rows, then reduced)
    const std::size_t flatSize = static_cast<std::size_t>(numDP) * (numDP + 1) / 2;
    std::vector<double> flatLocal(flatSize, 0.0);
    std::vector<double> flatGlobal(flatSize, 0.0);

    for (int i = rank; i < numDP; i += size) {
        for (int j = 0; j <= i; j++) {
            std::size_t idx = static_cast<std::size_t>(i) * (i + 1) / 2 + j;
            flatLocal[idx] = HaversineDistance(demandPoints[i][0], demandPoints[i][1],
                                               demandPoints[j][0], demandPoints[j][1]);
        }
    }

    MPI_Allreduce(flatLocal.data(), flatGlobal.data(), static_cast<int>(flatSize), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    distanceMatrix = new double*[numDP];
    for (int i = 0; i < numDP; ++i) {
        distanceMatrix[i] = new double[i + 1];
        for (int j = 0; j <= i; ++j) {
            std::size_t idx = static_cast<std::size_t>(i) * (i + 1) / 2 + j;
            distanceMatrix[i][j] = flatGlobal[idx];
        }
    }

    double t_matrix = getTime();

    // init X / bestX (serial)
    for (int i = 0; i < numX; i++) {
        X[i] = i;
        bestX[i] = i;
    }
    u = evaluateSolution(X);
    bestU = u;

    // partition combinations by rank
    double localBestU = -1.0;
    std::vector<int> localBestX(numX), localX(numX);
    for (int first = rank; first < numCL - (numX - 1); first += size) {
        for (int i = 0; i < numX; ++i) localX[i] = first + i;
        do {
            double u_local = evaluateSolution(localX.data());
            if (u_local > localBestU) { localBestU = u_local; localBestX = localX; }
        } while (increaseX(localX.data(), numX - 1, numCL) && localX[0] == first);
    }

    struct { double val; int rank; } in{localBestU, rank}, out;
    MPI_Allreduce(&in, &out, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);

    std::vector<int> globalBestX(numX);
    if (rank == out.rank) {
        MPI_Send(localBestX.data(), numX, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    if (rank == 0) {
        MPI_Recv(globalBestX.data(), numX, MPI_INT, out.rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Bcast(globalBestX.data(), numX, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&out.val, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    bestU = out.val;
    for (int i = 0; i < numX; ++i) bestX[i] = globalBestX[i];

    double t_finish = getTime();
    if (rank == 0) {
        cout << "Matricos skaiciavimo trukme: " << t_matrix - t_start << endl;
        cout << "Sprendinio paieskos trukme: " << t_finish - t_matrix << endl;
        cout << "Algoritmo vykdymo trukme: " << t_finish - t_start << endl;
        cout << "Geriausias sprendinys: ";
        for (int i=0; i<numX; i++) cout << bestX[i] << " ";
        cout << "(" << bestU << " procentai rinkos)" << endl;
        cout << "----------------------------------------" << endl;
    }

    RunResult res;
    res.type = config.type;
    res.threads = config.threads;
    res.numDP = numDP;
    res.numPF = numPF;
    res.numCL = numCL;
    res.numX = numX;
    res.matrixTime = t_matrix - t_start;
    res.searchTime = t_finish - t_matrix;
    res.totalTime = t_finish - t_start;
    res.bestUtility = bestU;
    res.bestSolution.assign(bestX, bestX + numX);
    return res;
}
