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

    std::string configPath = (argc > 1) ? argv[1] : "./yaml/config_hpc2.yaml";
    std::vector<RunConfig> runs;
    if (!loadConfig(configPath, runs) || runs.empty()) {
        if (rank == 0) std::cerr << "Nepavyko ikelti konfiguracijos failo " << configPath << '\n';
        MPI_Finalize();
        return 1;
    }

    RunConfig rc = runs.front(); // use only one config
    rc.type = 3;                 // enforce MPI mode

    if (rank == 0) {
        if (runs.size() > 1) {
            std::cout << "Demesio: naudojama tik pirma konfiguracija is failo.\n";
        }
        std::cout << "----------------------------------------\n";
        std::cout << "Leidziama MPI konfiguracija\n";
        std::cout << "--Tipas: " << getLabel(rc.type) << "\n--Rangai (threads laukas): " << rc.threads << "\n\n";
    }

    int size = 1;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0 && rc.threads != size) {
        std::cerr << "Perspejimas: konfiguroje nurodyta " << rc.threads
                  << " rangai, bet MPI paleista su " << size << ".\n";
    }

    runEnumerationMPI(rc);

    MPI_Finalize();
    return 0;
}
