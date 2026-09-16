#pragma once

#include "../core/types.h"
#include <string>
#include <vector>
#include <memory>

namespace samp_editor {

enum class ExportFormat {
    CreateDynamicObject_Streamer,
    CreateObject_Standard,
    RawCSV
};

struct EditorObject {
    uint32_t id{0};
    uint32_t modelId{0};
    std::string name;
    Vec3 position;
    Quat rotationQuat;
    Vec3 rotationEuler; // Degrees (rX, rY, rZ)
    int32_t worldId{-1};
    int32_t interiorId{-1};
    float streamDistance{300.0f};
    float drawDistance{300.0f};

    void SyncEulerFromQuat() {
        rotationEuler = rotationQuat.ToEulerGTA();
    }

    void SyncQuatFromEuler() {
        rotationQuat = Quat::FromEulerGTA(rotationEuler.x, rotationEuler.y, rotationEuler.z);
    }
};

class PawnExporter {
public:
    static std::string ExportToString(const std::vector<const EditorObject*>& objects, ExportFormat format = ExportFormat::CreateDynamicObject_Streamer);
    static std::string ExportToString(const std::vector<std::unique_ptr<EditorObject>>& objects, ExportFormat format = ExportFormat::CreateDynamicObject_Streamer);
    static std::string ExportToString(const std::vector<EditorObject>& objects, ExportFormat format = ExportFormat::CreateDynamicObject_Streamer);

    static bool ExportToFile(const std::string& filePath, const std::vector<std::unique_ptr<EditorObject>>& objects, ExportFormat format = ExportFormat::CreateDynamicObject_Streamer);
};

} // namespace samp_editor
