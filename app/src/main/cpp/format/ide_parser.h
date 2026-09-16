#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace samp_editor {

struct ObjectDef {
    uint32_t id{0};
    std::string modelName;
    std::string txdName;
    float drawDistance{100.0f};
    uint32_t flags{0};
    bool isTimed{false};
    uint32_t timeOn{0};
    uint32_t timeOff{0};
};

class IDEParser {
public:
    static bool ParseFile(const std::string& filePath, std::unordered_map<uint32_t, ObjectDef>& outDefs);
    static bool ParseString(const std::string& content, std::unordered_map<uint32_t, ObjectDef>& outDefs);

private:
    static std::vector<std::string> SplitCSV(const std::string& line);
    static std::string Trim(const std::string& str);
};

} // namespace samp_editor
