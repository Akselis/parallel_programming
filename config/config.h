#pragma once
#include <string>
#include <vector>

struct RunConfig {
    int type = 0;
    int threads = 1;
    int numDP = 6000;
    int numPF = 5;
    int numCL = 60;
    int numX = 3;
    std::string inputFile = "demandPoints.dat";
};

// Loads one or more run configurations from YAML.
bool loadConfig(const std::string& path, std::vector<RunConfig>& out);
std::string getLabel(int type);
