#include "flpenum/flpenum.h"
#include "config/config.h"
#include <mpi.h>
#include <iostream>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    RunConfig rc;          // use defaults
    rc.type = 3;           // distributed MPI search
    // rc.threads can be controlled via OMP_NUM_THREADS if desired

    if (rank == 0) {
        std::cout << "Paleidziamas MPI skaiciavimas su " << rc.threads << " gijomis per procesa." << std::endl;
    }

    runEnumerationMPI(rc);

    MPI_Finalize();
    return 0;
}
