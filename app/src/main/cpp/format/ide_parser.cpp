#include "ide_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace samp_editor {

std::string IDEParser::Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> IDEParser::SplitCSV(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        std::string trimmed = Trim(item);
        if (!trimmed.empty()) {
            tokens.push_back(trimmed);
        }
    }
    // Jika tidak ada koma, coba pisahkan dengan spasi/tab
    if (tokens.size() <= 1) {
        tokens.clear();
        std::stringstream ss2(line);
        while (ss2 >> item) {
            tokens.push_back(Trim(item));
        }
    }
    return tokens;
}

bool IDEParser::ParseFile(const std::string& filePath, std::unordered_map<uint32_t, ObjectDef>& outDefs) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    return ParseString(buffer.str(), outDefs);
}

bool IDEParser::ParseString(const std::string& content, std::unordered_map<uint32_t, ObjectDef>& outDefs) {
    std::stringstream ss(content);
    std::string line;
    std::string currentSection;

    while (std::getline(ss, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        if (lowerLine == "objs" || lowerLine == "tobj" || lowerLine == "anim") {
            currentSection = lowerLine;
            continue;
        } else if (lowerLine == "end") {
            currentSection.clear();
            continue;
        }

        if (currentSection == "objs") {
            auto tokens = SplitCSV(line);
            // Format: id, modelName, txdName, drawDistance, flags (atau id, modelName, txdName, meshCount, drawDist, flags)
            if (tokens.size() >= 4) {
                try {
                    ObjectDef def;
                    def.id = static_cast<uint32_t>(std::stoul(tokens[0]));
                    def.modelName = tokens[1];
                    def.txdName = tokens[2];

                    if (tokens.size() == 5) {
                        def.drawDistance = std::stof(tokens[3]);
                        def.flags = static_cast<uint32_t>(std::stoul(tokens[4]));
                    } else if (tokens.size() >= 6) {
                        // Beberapa IDE format lama menyertakan meshCount sebelum drawDistance
                        def.drawDistance = std::stof(tokens[tokens.size() - 2]);
                        def.flags = static_cast<uint32_t>(std::stoul(tokens[tokens.size() - 1]));
                    }

                    outDefs[def.id] = def;
                } catch (...) {
                    // Skip line parsing errors
                }
            }
        } else if (currentSection == "tobj") {
            auto tokens = SplitCSV(line);
            if (tokens.size() >= 6) {
                try {
                    ObjectDef def;
                    def.id = static_cast<uint32_t>(std::stoul(tokens[0]));
                    def.modelName = tokens[1];
                    def.txdName = tokens[2];
                    def.drawDistance = std::stof(tokens[3]);
                    def.flags = static_cast<uint32_t>(std::stoul(tokens[4]));
                    def.isTimed = true;
                    if (tokens.size() >= 7) {
                        def.timeOn = static_cast<uint32_t>(std::stoul(tokens[5]));
                        def.timeOff = static_cast<uint32_t>(std::stoul(tokens[6]));
                    }
                    outDefs[def.id] = def;
                } catch (...) {
                }
            }
        }
    }

    return !outDefs.empty();
}

} // namespace samp_editor
