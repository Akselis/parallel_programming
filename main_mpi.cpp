#include "flpenum/flpenum.h"
#include "config/config.h"
#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::string configPath = (argc > 1) ? argv[1] : "./yaml/config_hpc.yaml";
    int runIndex = (argc > 2) ? std::max(0, std::atoi(argv[2])) : 0;

    std::vector<RunConfig> runs;
    if (!loadConfig(configPath, runs) || runs.empty()) {
        if (rank == 0) std::cerr << "Nepavyko ikelti konfiguracijos failo " << configPath << '\n';
        MPI_Finalize();
        return 1;
    }
    if (runIndex >= static_cast<int>(runs.size())) {
        if (rank == 0) std::cerr << "Neteisingas run indeksas " << runIndex << " (viso " << runs.size() << ")\n";
        MPI_Finalize();
        return 1;
    }

    RunConfig rc = runs[runIndex];
    rc.type = 3; // enforce MPI mode

    if (rank == 0) {
        std::cout << "----------------------------------------\n";
        std::cout << "Leidziama MPI konfiguracija nr. " << (runIndex + 1) << "/" << runs.size() << "\n";
        std::cout << "--Tipas: " << getLabel(rc.type) << "\n--Rangai: " << rc.threads << "\n\n";
    }

    runEnumerationMPI(rc);

    MPI_Finalize();
    return 0;
}
