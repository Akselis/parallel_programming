#include "config.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

bool loadConfig(const std::string& path, std::vector<RunConfig>& out) {
    try {
        YAML::Node cfg = YAML::LoadFile(path);

        auto parseOne = [](const YAML::Node& n) {
            RunConfig rc;
            if (n["type"])      rc.type      = n["type"].as<int>();
            if (n["threads"])   rc.threads   = n["threads"].as<int>();
            if (n["numDP"])     rc.numDP     = n["numDP"].as<int>();
            if (n["numPF"])     rc.numPF     = n["numPF"].as<int>();
            if (n["numCL"])     rc.numCL     = n["numCL"].as<int>();
            if (n["numX"])      rc.numX      = n["numX"].as<int>();
            if (n["inputFile"]) rc.inputFile = n["inputFile"].as<std::string>();
            return rc;
        };

        if (cfg["runconfig"]) {
            const auto& runs = cfg["runconfig"];
            if (runs.IsSequence()) {
                for (const auto& n : runs) out.push_back(parseOne(n));
            } else if (runs.IsMap()) {
                out.push_back(parseOne(runs));
            }
        } else if (cfg.IsMap()) {
            out.push_back(parseOne(cfg));
        }

        if (out.empty()) {
            std::cerr << "Config load failed: no runconfig entries found\n";
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Config load failed: " << e.what() << '\n';
        return false;
    }
}

std::string getLabel(int type) {
    switch (type) {
        case 0: return "Nelygiagretintas";
        case 1: return "Lygiagretintas algoritmas, kai duomenu ikelimas ir matricos skaiciavimas nuoseklus";
        case 2: return "Lygiagretintas atstumus matricos skaiciavimas, kai like veiksmai nuoseklus";
        case 3: return "Lygiagretintas matricos skaiciavimas ir paieska";
        default: return "Nezinomas tipas";
    }
}
