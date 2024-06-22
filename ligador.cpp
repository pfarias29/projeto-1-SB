#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>

struct Symbol {
    std::string name;
    int address;
};

struct Relocation {
    int position;
    int value;
};

struct Module {
    std::unordered_map<int, std::string> uso;
    std::unordered_map<std::string, int> def;
    std::unordered_map<int, int> code;
    std::vector<bool> relocations;
    int baseAddress;
};

Module parseModule(const std::string& filename) {
    Module module;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return module;
    }

    std::string line;
    while (getline(file, line)) {
        if (line.find("USO") != std::string::npos) {
            while (getline(file, line) && !line.empty()) {
                std::istringstream iss(line);
                std::string symbol;
                int address;
                iss >> symbol >> address;
                module.uso[address] = symbol;
            }
        } else if (line.find("DEF") != std::string::npos) {
            while (getline(file, line) && !line.empty()) {
                std::istringstream iss(line);
                std::string symbol;
                int address;
                iss >> symbol >> address;
                module.def[symbol] = address;
            }
        } else if (line.find("REAL") != std::string::npos) {
            getline(file, line);
            for (char c : line) {
                module.relocations.push_back(c == '1');
            }
        } else {
            if (!line.empty()) {
                std::istringstream iss(line);
                int value;
                int address = 0;
                while (iss >> value) {
                    module.code[address] = value;
                    address++;
                }
                module.baseAddress = module.code.size();
            }
        }
    }

    file.close();
    return module;
}

void linkModules(const Module& mod1, const Module& mod2, Module& output) {
    for (const auto& code : mod1.code) {
        output.code[code.first] = code.second;
    }   
    for (const auto& code : mod2.code) {
        output.code[code.first + mod1.baseAddress] = code.second;
    }

    // Resolver símbolos
    std::unordered_map<std::string, int> symbolTable;
    for (const auto& def : mod1.def) {
        symbolTable[def.first] = def.second;
    }
    for (const auto& def : mod2.def) {
        symbolTable[def.first] = def.second + mod1.baseAddress;
    }

    // Combina relocações e ajusta endereços
    for (const auto& rel : mod1.relocations) {
        output.relocations.push_back(rel);
    }
    for (const auto& rel : mod2.relocations) {
        output.relocations.push_back(rel);
    }

    // Resolve as referências de uso
    for (const auto& uso : mod1.uso) {
        auto symbol = symbolTable.find(uso.second);
        if (symbol != symbolTable.end()) {
            output.code[uso.first] += symbol->second;
            output.relocations[uso.first] = false;
        }
    }
    for (const auto& uso : mod2.uso) {
        auto symbol = symbolTable.find(uso.second);
        if (symbol != symbolTable.end()) {
            output.code[uso.first + mod1.baseAddress] += symbol->second;
            output.relocations[uso.first + mod1.baseAddress] = false;
        }
    }

    // Ajusta endereços 
    int size = output.code.size();

    for (int i = 0; i < size; i++) {
        if (output.relocations[i]) {
            if (i <= mod1.baseAddress)
                output.code[i] += 0;
            else
                output.code[i] += mod1.baseAddress;
        }
    }
}

void writeOutput(const std::string& filename, const Module& module) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Erro ao criar o arquivo de saída: " << filename << std::endl;
        return;
    }

    int size = module.code.size();

    for (int i = 0; i < size; i++) {
        file << module.code.at(i) << " ";
    }

    file.close();
}

int main(int argc, char** argv) {
    if (argc == 3) {
        Module mod1 = parseModule(argv[1]);
        Module mod2 = parseModule(argv[2]);
        Module linkedModule;
        linkModules(mod1, mod2, linkedModule);
        writeOutput("prog1.e", linkedModule);
    } else {
        std::cerr << "Número de argumentos inválido" << std::endl;
    }

    return 0;
}
