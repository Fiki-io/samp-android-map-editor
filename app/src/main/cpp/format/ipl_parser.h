#pragma once

#include "../core/types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace samp_editor {

struct MapInstance {
    uint32_t modelId{0};
    std::string modelName;
    int32_t interior{0};
    Vec3 position;
    Quat rotation; // Quaternion (qx, qy, qz, qw)
    int32_t lodIndex{-1};
};

class IPLParser {
public:
    static bool ParseFile(const std::string& filePath, std::vector<MapInstance>& outInstances);
    static bool ParseString(const std::string& content, std::vector<MapInstance>& outInstances);

private:
    static std::vector<std::string> SplitCSV(const std::string& line);
    static std::string Trim(const std::string& str);
};

} // namespace samp_editor
