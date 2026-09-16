#pragma once

#include "../core/types.h"
#include "../archive/rw_stream.h"
#include <string>
#include <vector>
#include <cstdint>

namespace samp_editor {

struct DFFVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    uint32_t color{0xFFFFFFFF};
};

struct DFFSubMesh {
    std::string textureName;
    uint32_t materialColor{0xFFFFFFFF};
    std::vector<uint32_t> indices;
};

struct DFFGeometry {
    std::vector<DFFVertex> vertices;
    std::vector<DFFSubMesh> subMeshes;
    AABB boundingBox;
};

struct DFFModel {
    std::string name;
    std::vector<DFFGeometry> geometries;
    AABB boundingBox;
    bool isValid{false};
};

class DFFLoader {
public:
    static bool LoadFromMemory(const uint8_t* data, size_t size, DFFModel& outModel, const std::string& modelName = "");

private:
    static bool ParseClump(RwStreamReader& reader, size_t clumpSize, DFFModel& outModel);
    static bool ParseGeometryList(RwStreamReader& reader, size_t listSize, std::vector<DFFGeometry>& outGeoms);
    static bool ParseGeometry(RwStreamReader& reader, size_t geomSize, DFFGeometry& outGeom);
    static bool ParseMaterialList(RwStreamReader& reader, size_t matListSize, std::vector<std::string>& outTextureNames);
    static bool ParseMaterial(RwStreamReader& reader, size_t matSize, std::string& outTextureName);
    static bool ParseBinMeshExtension(RwStreamReader& reader, size_t extSize, const std::vector<std::string>& texNames, DFFGeometry& outGeom);
};

} // namespace samp_editor
