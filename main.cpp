#include "flpenum/flpenum.h"
#include "config/config.h"
#include <omp.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <mpi.h>

int main(int argc, char** argv)
{
    std::string configPath = (argc > 1) ? argv[1] : "./yaml/config.yaml";
    std::vector<RunConfig> runs;
    if (!loadConfig(configPath, runs)) {
        std::cerr << "Nepavyko ikelti konfiguracijos failo " << configPath << '\n';
        return 1;
    }

    std::vector<RunResult> results;
    results.reserve(runs.size());

    std::cout << "----------------------------------------" << "\n";

    bool mpiInitialized = false;
    for (size_t i = 0; i < runs.size(); ++i) {
        const auto& rc = runs[i];
        std::cout << "Leidziama konfiguracija nr. " << (i + 1) << "/" << runs.size() << "\n";
        std::cout << "--Tipas: " << getLabel(rc.type) << "\n--Gijos: " << rc.threads << "\n\n";
        if(rc.type == 3 && !mpiInitialized){
            mpiInitialized = true;
            int rank = 0, size = 1;
            MPI_Init(nullptr, nullptr);
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);
        }
        results.push_back(runEnumeration(rc));
    }
    if(mpiInitialized){
        MPI_Finalize();
    }

    std::filesystem::create_directories("results");
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::ostringstream name;
    name << "results/results-" << std::put_time(&tm, "%Y%m%d-%H%M") << ".txt";

    std::ofstream out(name.str());
    if (!out) {
        std::cerr << "Nepavyko atverti '" << name.str() << "' rasymui\n";
        return 1;
    }
    out << std::fixed << std::setprecision(6);
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        out << "------------------------------\n";
        out << "Tipas: " << getLabel(r.type) << "\n";
        out << "Gijos: " << r.threads << "\n";
        out << "numDP: " << r.numDP << "\n";
        out << "numPF: " << r.numPF << "\n";
        out << "numCL: " << r.numCL << "\n";
        out << "numX: " << r.numX << "\n";
        out << "Matricos skaiciavimo trukme: " << r.matrixTime << "\n";
        out << "Sprendinio paieskos trukme: " << r.searchTime << "\n";
        out << "Algoritmo vykdymo trukme: " << r.totalTime << "\n";
        out << "Geriausias sprendinys: ";
        for (size_t j = 0; j < r.bestSolution.size(); ++j) {
            out << r.bestSolution[j] << (j + 1 < r.bestSolution.size() ? " " : " ");
        }
        out << "(" << r.bestUtility << " procentai rinkos)\n";
    }
    out << "------------------------------\n";
    return 0;
}
