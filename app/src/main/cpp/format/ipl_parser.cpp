#include "ipl_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace samp_editor {

std::string IPLParser::Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> IPLParser::SplitCSV(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        std::string trimmed = Trim(item);
        if (!trimmed.empty()) {
            tokens.push_back(trimmed);
        }
    }
    if (tokens.size() <= 1) {
        tokens.clear();
        std::stringstream ss2(line);
        while (ss2 >> item) {
            tokens.push_back(Trim(item));
        }
    }
    return tokens;
}

bool IPLParser::ParseFile(const std::string& filePath, std::vector<MapInstance>& outInstances) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    return ParseString(buffer.str(), outInstances);
}

bool IPLParser::ParseString(const std::string& content, std::vector<MapInstance>& outInstances) {
    std::stringstream ss(content);
    std::string line;
    std::string currentSection;

    while (std::getline(ss, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        if (lowerLine == "inst") {
            currentSection = "inst";
            continue;
        } else if (lowerLine == "end") {
            currentSection.clear();
            continue;
        }

        if (currentSection == "inst") {
            auto tokens = SplitCSV(line);
            // Format: id, modelName, interior, posX, posY, posZ, rotX, rotY, rotZ, rotW, lodIndex
            if (tokens.size() >= 10) {
                try {
                    MapInstance inst;
                    inst.modelId = static_cast<uint32_t>(std::stoul(tokens[0]));
                    inst.modelName = tokens[1];
                    inst.interior = std::stoi(tokens[2]);
                    inst.position.x = std::stof(tokens[3]);
                    inst.position.y = std::stof(tokens[4]);
                    inst.position.z = std::stof(tokens[5]);

                    // Quaternion orientation
                    inst.rotation.x = std::stof(tokens[6]);
                    inst.rotation.y = std::stof(tokens[7]);
                    inst.rotation.z = std::stof(tokens[8]);
                    inst.rotation.w = std::stof(tokens[9]);

                    if (tokens.size() >= 11) {
                        inst.lodIndex = std::stoi(tokens[10]);
                    }

                    outInstances.push_back(inst);
                } catch (...) {
                    // Skip parsing errors
                }
            }
        }
    }

    return !outInstances.empty();
}

} // namespace samp_editor
