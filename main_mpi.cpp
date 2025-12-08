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
    std::vector<RunConfig> runs;
    if (!loadConfig(configPath, runs)) {
        if (rank == 0) std::cerr << "Nepavyko ikelti konfiguracijos failo " << configPath << '\n';
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        std::cout << "----------------------------------------\n";
    }

    for (size_t i = 0; i < runs.size(); ++i) {
        RunConfig rc = runs[i];
        if (rc.type != 3) {
            if (rank == 0) {
                std::cout << "Praleidziama konfiguracija nr. " << (i + 1)
                          << " (tipas " << rc.type << " neskirtas MPI)\n";
            }
            continue;
        }

        if (rank == 0) {
            std::cout << "Leidziama MPI konfiguracija nr. " << (i + 1) << "/" << runs.size() << "\n";
            std::cout << "--Tipas: " << getLabel(rc.type) << "\n--Gijos: " << rc.threads << "\n\n";
        }

        runEnumerationMPI(rc);
    }

    MPI_Finalize();
    return 0;
}
