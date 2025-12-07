#include "flpenum.h"
#include <iostream>
#include <cmath>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
#include <mpi.h>

using namespace std;

//===== Globalus kintamieji ===================================================

int numDP = 6000;            // Vietoviu skaicius (demand points, max 10000)
int numPF = 5;               // Esanciu objektu skaicius (preexisting facilities)
int numCL = 60;              // Kandidatu naujiems objektams skaicius (candidate locations)
int numX  = 3;               // Nauju objektu skaicius
std::string demandFile = "demandPoints.dat";

double **demandPoints;       // Geografiniai duomenys
double **distanceMatrix;   	 // Masyvas atstumu matricai saugoti

int *X = nullptr;            // Naujas sprendinys
int *bestX = nullptr;        // Geriausias rastas sprendinys
double u, bestU;             // Naujo sprendinio ir geriausio sprendinio naudingumas (utility)

//===== Funkciju prototipai ===================================================

double getTime();
void loadDemandPoints();
double HaversineDistance(double lat1, double lon1, double lat2, double lon2);
double HaversineDistance(int i, int j);
double evaluateSolution(const int* X);
int increaseX(int* X, int index, int maxindex);

//=============================================================================

RunResult runEnumeration(const RunConfig& config) {
	numDP = config.numDP;
	numPF = config.numPF;
	numCL = config.numCL;
	numX  = config.numX;
	demandFile = config.inputFile;
	omp_set_num_threads(config.threads);

	int mpiRankLocal = 0;
	if (config.type == 3) {
		int mpiInitFlag = 0;
		MPI_Initialized(&mpiInitFlag);
		if (mpiInitFlag) {
			MPI_Comm_rank(MPI_COMM_WORLD, &mpiRankLocal);
		}
	}

	int mpiRank = 0, mpiSize = 1;
	int mpiInitialized = 0;
	if (config.type == 3) {
		MPI_Initialized(&mpiInitialized);
		if (!mpiInitialized) {
			MPI_Init(nullptr, nullptr);
			mpiInitialized = 1;
		}
		MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
		MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
	}

	// (Re)allocate solution buffers for current numX
	if (X) delete[] X;
	if (bestX) delete[] bestX;
	X = new int[numX];
	bestX = new int[numX];

	if (config.type == 3 && mpiRank != 0) {
		// allocate demandPoints; data will come via broadcast
		demandPoints = new double*[numDP];
		for (int i = 0; i < numDP; ++i) demandPoints[i] = new double[3];
	} else {
		loadDemandPoints();             // Duomenu nuskaitymas is failo
	}

	// Broadcast demandPoints to all ranks for type 3
	if (config.type == 3) {
		std::vector<double> flat(numDP * 3);
		if (mpiRank == 0) {
			for (int i = 0; i < numDP; ++i) {
				flat[i*3 + 0] = demandPoints[i][0];
				flat[i*3 + 1] = demandPoints[i][1];
				flat[i*3 + 2] = demandPoints[i][2];
			}
		}
		MPI_Bcast(flat.data(), static_cast<int>(flat.size()), MPI_DOUBLE, 0, MPI_COMM_WORLD);
		if (mpiRank != 0) {
			for (int i = 0; i < numDP; ++i) {
				demandPoints[i][0] = flat[i*3 + 0];
				demandPoints[i][1] = flat[i*3 + 1];
				demandPoints[i][2] = flat[i*3 + 2];
			}
		}
	}

    double t_start = getTime();     // Algoritmo vykdymo pradzios laikas

    //----- Atstumu matricos skaiciavimas -------------------------------------
    distanceMatrix = new double*[numDP];

	switch (config.type) {
	case 2:
	case 3:
		#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < numDP; i++) {
			distanceMatrix[i] = new double[i + 1];
			for (int j = 0; j <= i; j++) {
				distanceMatrix[i][j] = HaversineDistance(demandPoints[i][0], demandPoints[i][1], demandPoints[j][0], demandPoints[j][1]);
			}
		}
		break;
	case 0:
	case 1:
	default:
		for (int i = 0; i < numDP; i++) {
			distanceMatrix[i] = new double[i + 1];
			for (int j = 0; j <= i; j++) {
				distanceMatrix[i][j] = HaversineDistance(demandPoints[i][0], demandPoints[i][1], demandPoints[j][0], demandPoints[j][1]);
			}
		}
		break;
	}

    double t_matrix = getTime();
    cout << "Matricos skaiciavimo trukme: " << t_matrix - t_start << endl;

    //----- Pradines naujo ir geriausio sprendiniu reiksmes -------------------
	switch (config.type) {
	case 1:
		#pragma omp parallel for schedule(static)
		for (int i = 0; i < numX; i++) {
			X[i] = i;
			bestX[i] = i;
		}
		break;
	case 0:
	case 2:
	case 3:
	default:
		for (int i = 0; i < numX; i++) {
			X[i] = i;
			bestX[i] = i;
		}
		break;
	}
    u = evaluateSolution(X);        // Naujo sprendinio naudingumas (utility)
    bestU = u;                      // Geriausio sprendinio sprendinio naudingumas
		
    //----- Visu galimu sprendiniu perrinkimas --------------------------------
	switch (config.type) {
	case 1:{
		double globalBestU = -1.0;
		std::vector<int> globalBestX(numX);

		#pragma omp parallel
		{
			std::vector<int> localX(numX), localBestX(numX);
			double localBestU = -1.0;

			// Assign disjoint first-element ranges to threads
			int tid = omp_get_thread_num();
			int nt  = omp_get_num_threads();
			for (int first = tid; first < numCL - (numX - 1); first += nt) {
				// init combination starting with `first`
				for (int i = 0; i < numX; ++i) localX[i] = first + i;
				// iterate combinations whose first element == first
				do {
					double u_local = evaluateSolution(localX.data());
					if (u_local > localBestU) {
						localBestU = u_local;
						localBestX = localX;
					}
				} while (increaseX(localX.data(), numX - 1, numCL) && localX[0] == first);
			}

			// Reduce to global best
			#pragma omp critical
			{
				if (localBestU > globalBestU) {
					globalBestU = localBestU;
					globalBestX = localBestX;
				}
			}
		}

		if (globalBestU > bestU) {
			bestU = globalBestU;
			for (int i = 0; i < numX; ++i) bestX[i] = globalBestX[i];
		}
		break;
	}
	case 3:{

		double localBestU = -1.0;
		std::vector<int> localBestX(numX), localX(numX);
		for (int first = mpiRank; first < numCL - (numX - 1); first += mpiSize) {
			for (int i = 0; i < numX; ++i) localX[i] = first + i;
			do {
				double u_local = evaluateSolution(localX.data());
				if (u_local > localBestU) { localBestU = u_local; localBestX = localX; }
			} while (increaseX(localX.data(), numX - 1, numCL) && localX[0] == first);
		}

		struct { double val; int rank; } in{localBestU, mpiRank}, out;
		MPI_Allreduce(&in, &out, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);
		std::vector<int> globalBestX(numX);
		if (mpiRank == out.rank) MPI_Send(localBestX.data(), numX, MPI_INT, 0, 0, MPI_COMM_WORLD);
		if (mpiRank == 0) MPI_Recv(globalBestX.data(), numX, MPI_INT, out.rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Bcast(globalBestX.data(), numX, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&out.val, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		bestU = out.val;
		for (int i = 0; i < numX; ++i) bestX[i] = globalBestX[i];
		break;
	}
	case 0:
	case 2:
	default:
		while (increaseX(X, numX - 1, numCL) == true) {
			u = evaluateSolution(X);
			if (u > bestU) {
				bestU = u;
				for (int i = 0; i < numX; i++) bestX[i] = X[i];
			}
		}
		break;
	}
	
    //----- Rezultatu spausdinimas --------------------------------------------
	double t_finish = getTime();     // Skaiciavimu pabaigos laikas
	if (config.type != 3 || mpiRankLocal == 0) {
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

//===== Funkciju implementacijos (siu funkciju LYGIAGRETINTI NEREIKIA) ========

void loadDemandPoints() {
	FILE *f;
	f = fopen(demandFile.c_str(), "r");
	demandPoints = new double*[numDP];
	for (int i=0; i<numDP; i++) {
		demandPoints[i] = new double[3];
		fscanf(f, "%lf%lf%lf", &demandPoints[i][0], &demandPoints[i][1], &demandPoints[i][2]);
	}
	fclose(f);
}

//=============================================================================

double HaversineDistance(double lat1, double lon1, double lat2, double lon2) {
	double dlat = fabs(lat1 - lat2);
	double dlon = fabs(lon1 - lon2);
	double aa = pow((sin((double)dlat/(double)2*0.01745)),2) + cos(lat1*0.01745) *
               cos(lat2*0.01745) * pow((sin((double)dlon/(double)2*0.01745)),2);
	double c = 2 * atan2(sqrt(aa), sqrt(1-aa));
	double d = 6371 * c; 
	return d;
}

double HaversineDistance(int i, int j) {
	if (i >= j)	return distanceMatrix[i][j];
	else return distanceMatrix[j][i];
}

//=============================================================================

double getTime() {
   struct timeval laikas;
   gettimeofday(&laikas, NULL);
   double rez = (double)laikas.tv_sec+(double)laikas.tv_usec/1000000;
   return rez;
}

//=============================================================================

double evaluateSolution(const int *X) {
        double U = 0.0;
        double totalU = 0.0;

        for (int i = 0; i < numDP; ++i) {
            double w = demandPoints[i][2];
            totalU += w;
    
            double bestPF = 1e300;
            for (int j = 0; j < numPF; ++j) {
                double d = HaversineDistance(i, j);
                if (d < bestPF) bestPF = d;
            }
    
            double bestX = 1e300;
            for (int j = 0; j < numX; ++j) {
                double d = HaversineDistance(i, X[j]);
                if (d < bestX) bestX = d;
            }
    
            if (bestX < bestPF)       U += w;
            else if (bestX == bestPF) U += 0.3 * w;
        }
    
        return U / totalU * 100.0;
    }

//=============================================================================

int increaseX(int *X, int index, int maxindex) {
	if (X[index]+1 < maxindex-(numX-index-1)) {
		X[index]++;
	}
	else {		 
		if ((index == 0) && (X[index]+1 == maxindex-(numX-index-1))) {
			return 0;
		}
		else {
			if (increaseX(X, index-1, maxindex)) X[index] = X[index-1]+1;
			else return 0;
		}	
	}
	return 1;
}
