#include "dff_loader.h"
#include <iostream>
#include <unordered_map>

namespace samp_editor {

namespace {
    constexpr uint32_t rwGEOMETRYTRISTRIP = 0x0001;
    constexpr uint32_t rwGEOMETRYPOSITIONS = 0x0002;
    constexpr uint32_t rwGEOMETRYTEXTURED = 0x0004;
    constexpr uint32_t rwGEOMETRYPRELIT = 0x0008;
    constexpr uint32_t rwGEOMETRYNORMALS = 0x0010;
    constexpr uint32_t rwGEOMETRYLIGHT = 0x0020;
    constexpr uint32_t rwGEOMETRYMODULATE = 0x0040;
    constexpr uint32_t rwGEOMETRYTEXTURED2 = 0x0080;

#pragma pack(push, 1)
    struct RwTriangle {
        uint16_t b;
        uint16_t a;
        uint16_t materialIndex;
        uint16_t c;
    };
#pragma pack(pop)
}

bool DFFLoader::LoadFromMemory(const uint8_t* data, size_t size, DFFModel& outModel, const std::string& modelName) {
    outModel = DFFModel();
    outModel.name = modelName;

    RwStreamReader reader(data, size);
    RwChunkHeader header{};

    while (reader.ReadHeader(header)) {
        size_t nextChunk = reader.GetOffset() + header.size;
        if (header.type == rwID_CLUMP) {
            if (ParseClump(reader, header.size, outModel)) {
                outModel.isValid = true;
            }
        }
        reader.SetOffset(nextChunk);
    }

    // Hitung total bounding box
    for (const auto& g : outModel.geometries) {
        outModel.boundingBox.min.x = std::min(outModel.boundingBox.min.x, g.boundingBox.min.x);
        outModel.boundingBox.min.y = std::min(outModel.boundingBox.min.y, g.boundingBox.min.y);
        outModel.boundingBox.min.z = std::min(outModel.boundingBox.min.z, g.boundingBox.min.z);

        outModel.boundingBox.max.x = std::max(outModel.boundingBox.max.x, g.boundingBox.max.x);
        outModel.boundingBox.max.y = std::max(outModel.boundingBox.max.y, g.boundingBox.max.y);
        outModel.boundingBox.max.z = std::max(outModel.boundingBox.max.z, g.boundingBox.max.z);
    }

    return outModel.isValid;
}

bool DFFLoader::ParseClump(RwStreamReader& reader, size_t clumpSize, DFFModel& outModel) {
    size_t endOffset = reader.GetOffset() + clumpSize;

    while (reader.GetOffset() < endOffset) {
        RwChunkHeader header{};
        if (!reader.ReadHeader(header)) break;

        size_t nextChunk = reader.GetOffset() + header.size;
        if (header.type == rwID_GEOMETRYLIST) {
            ParseGeometryList(reader, header.size, outModel.geometries);
        }
        reader.SetOffset(nextChunk);
    }

    return !outModel.geometries.empty();
}

bool DFFLoader::ParseGeometryList(RwStreamReader& reader, size_t listSize, std::vector<DFFGeometry>& outGeoms) {
    size_t endOffset = reader.GetOffset() + listSize;

    RwChunkHeader structHeader{};
    if (!reader.ReadHeader(structHeader) || structHeader.type != rwID_STRUCT) return false;

    uint32_t geomCount = 0;
    if (!reader.Read(geomCount)) return false;
    reader.SetOffset(reader.GetOffset() + (structHeader.size - sizeof(uint32_t)));

    outGeoms.reserve(geomCount);

    while (reader.GetOffset() < endOffset) {
        RwChunkHeader header{};
        if (!reader.ReadHeader(header)) break;

        size_t nextChunk = reader.GetOffset() + header.size;
        if (header.type == rwID_GEOMETRY) {
            DFFGeometry geom;
            if (ParseGeometry(reader, header.size, geom)) {
                outGeoms.push_back(std::move(geom));
            }
        }
        reader.SetOffset(nextChunk);
    }

    return !outGeoms.empty();
}

bool DFFLoader::ParseGeometry(RwStreamReader& reader, size_t geomSize, DFFGeometry& outGeom) {
    size_t endOffset = reader.GetOffset() + geomSize;

    RwChunkHeader structHeader{};
    if (!reader.ReadHeader(structHeader) || structHeader.type != rwID_STRUCT) return false;

    size_t structEnd = reader.GetOffset() + structHeader.size;

    uint32_t formatFlags = 0;
    uint32_t numTriangles = 0;
    uint32_t numVertices = 0;
    uint32_t numMorphTargets = 0;

    if (!reader.Read(formatFlags) || !reader.Read(numTriangles) ||
        !reader.Read(numVertices) || !reader.Read(numMorphTargets)) {
        return false;
    }

    outGeom.vertices.resize(numVertices);

    // 1. Vertex Colors (Pre-lit)
    if (formatFlags & rwGEOMETRYPRELIT) {
        for (uint32_t i = 0; i < numVertices; ++i) {
            uint32_t col = 0;
            reader.Read(col);
            outGeom.vertices[i].color = col;
        }
    }

    // 2. Texture Coordinates (UV)
    if (formatFlags & (rwGEOMETRYTEXTURED | rwGEOMETRYTEXTURED2)) {
        for (uint32_t i = 0; i < numVertices; ++i) {
            float u = 0.0f, v = 0.0f;
            reader.Read(u);
            reader.Read(v);
            outGeom.vertices[i].uv = {u, v};
        }
    }

    // 3. Triangles (RwTriangle is always present in struct if numTriangles > 0)
    std::vector<RwTriangle> rawTriangles;
    if (numTriangles > 0) {
        rawTriangles.resize(numTriangles);
        reader.ReadBytes(rawTriangles.data(), numTriangles * sizeof(RwTriangle));
    }

    // 4. Morph Target (Posisi & Normal)
    for (uint32_t m = 0; m < numMorphTargets; ++m) {
        float boundingSphere[4]; // cx, cy, cz, radius
        reader.ReadBytes(boundingSphere, sizeof(boundingSphere));

        uint32_t hasVertices = 0, hasNormals = 0;
        reader.Read(hasVertices);
        reader.Read(hasNormals);

        if (hasVertices) {
            for (uint32_t i = 0; i < numVertices; ++i) {
                Vec3 p;
                reader.Read(p.x);
                reader.Read(p.y);
                reader.Read(p.z);
                outGeom.vertices[i].position = p;
                outGeom.boundingBox.Expand(p);
            }
        }

        if (hasNormals) {
            for (uint32_t i = 0; i < numVertices; ++i) {
                Vec3 n;
                reader.Read(n.x);
                reader.Read(n.y);
                reader.Read(n.z);
                outGeom.vertices[i].normal = n;
            }
        }
    }

    // Fallback: Generate normals if geometry has no normals
    bool allNormalsZero = true;
    for (const auto& v : outGeom.vertices) {
        if (v.normal.LengthSq() > 0.001f) {
            allNormalsZero = false;
            break;
        }
    }
    if (allNormalsZero && !rawTriangles.empty()) {
        for (const auto& tri : rawTriangles) {
            if (tri.a < outGeom.vertices.size() && tri.b < outGeom.vertices.size() && tri.c < outGeom.vertices.size()) {
                Vec3 v0 = outGeom.vertices[tri.a].position;
                Vec3 v1 = outGeom.vertices[tri.b].position;
                Vec3 v2 = outGeom.vertices[tri.c].position;
                Vec3 fn = (v1 - v0).Cross(v2 - v0);
                float len = fn.Length();
                if (len > 1e-6f) fn = fn * (1.0f / len);
                outGeom.vertices[tri.a].normal += fn;
                outGeom.vertices[tri.b].normal += fn;
                outGeom.vertices[tri.c].normal += fn;
            }
        }
        for (auto& v : outGeom.vertices) {
            float len = v.normal.Length();
            if (len > 1e-6f) {
                v.normal = v.normal * (1.0f / len);
            } else {
                v.normal = Vec3(0.0f, 0.0f, 1.0f);
            }
        }
    }

    reader.SetOffset(structEnd);

    // Baca sub-chunks: MaterialList dan Extension
    std::vector<std::string> textureNames;
    bool hasBinMesh = false;

    while (reader.GetOffset() < endOffset) {
        RwChunkHeader header{};
        if (!reader.ReadHeader(header)) break;

        size_t nextChunk = reader.GetOffset() + header.size;
        if (header.type == rwID_MATLIST) {
            ParseMaterialList(reader, header.size, textureNames);
        } else if (header.type == rwID_EXTENSION) {
            if (ParseBinMeshExtension(reader, header.size, textureNames, outGeom)) {
                hasBinMesh = true;
            }
        }
        reader.SetOffset(nextChunk);
    }

    // Fallback: Jika tidak ada BinMesh extension, bangun SubMesh dari rawTriangles
    if (!hasBinMesh && !rawTriangles.empty()) {
        std::unordered_map<uint16_t, DFFSubMesh> matSubmeshes;
        for (const auto& tri : rawTriangles) {
            auto& sm = matSubmeshes[tri.materialIndex];
            sm.indices.push_back(tri.a);
            sm.indices.push_back(tri.b);
            sm.indices.push_back(tri.c);
        }

        for (auto& [matIdx, sm] : matSubmeshes) {
            if (matIdx < textureNames.size()) {
                sm.textureName = textureNames[matIdx];
            }
            outGeom.subMeshes.push_back(std::move(sm));
        }
    }

    return !outGeom.vertices.empty();
}

bool DFFLoader::ParseMaterialList(RwStreamReader& reader, size_t matListSize, std::vector<std::string>& outTextureNames) {
    size_t endOffset = reader.GetOffset() + matListSize;

    RwChunkHeader structHeader{};
    if (!reader.ReadHeader(structHeader) || structHeader.type != rwID_STRUCT) return false;

    uint32_t materialCount = 0;
    if (!reader.Read(materialCount)) return false;
    reader.SetOffset(reader.GetOffset() + (structHeader.size - sizeof(uint32_t)));

    outTextureNames.reserve(materialCount);

    while (reader.GetOffset() < endOffset) {
        RwChunkHeader header{};
        if (!reader.ReadHeader(header)) break;

        size_t nextChunk = reader.GetOffset() + header.size;
        if (header.type == rwID_MATERIAL) {
            std::string texName;
            ParseMaterial(reader, header.size, texName);
            outTextureNames.push_back(texName);
        }
        reader.SetOffset(nextChunk);
    }

    return true;
}

bool DFFLoader::ParseMaterial(RwStreamReader& reader, size_t matSize, std::string& outTextureName) {
    size_t endOffset = reader.GetOffset() + matSize;

    RwChunkHeader structHeader{};
    if (!reader.ReadHeader(structHeader) || structHeader.type != rwID_STRUCT) return false;

    size_t structEnd = reader.GetOffset() + structHeader.size;

    // Material struct:
    // uint32_t flags, uint32_t color, uint32_t unused, uint32_t isTextured, float ambient, float specular, float diffuse
    reader.Skip(12); // flags, color, unused
    uint32_t isTextured = 0;
    reader.Read(isTextured);
    reader.SetOffset(structEnd);

    if (isTextured) {
        while (reader.GetOffset() < endOffset) {
            RwChunkHeader header{};
            if (!reader.ReadHeader(header)) break;

            size_t nextChunk = reader.GetOffset() + header.size;
            if (header.type == rwID_TEXTURE) {
                size_t texEnd = reader.GetOffset() + header.size;
                // Texture chunk berisi: Struct, String (diffuse name), String (alpha name)
                while (reader.GetOffset() < texEnd) {
                    RwChunkHeader sub{};
                    if (!reader.ReadHeader(sub)) break;
                    size_t subNext = reader.GetOffset() + sub.size;
                    if (sub.type == rwID_STRING) {
                        outTextureName = reader.ReadString(sub.size);
                        break;
                    }
                    reader.SetOffset(subNext);
                }
            }
            reader.SetOffset(nextChunk);
        }
    }

    return true;
}

bool DFFLoader::ParseBinMeshExtension(RwStreamReader& reader, size_t extSize, const std::vector<std::string>& texNames, DFFGeometry& outGeom) {
    size_t endOffset = reader.GetOffset() + extSize;
    bool found = false;

    while (reader.GetOffset() < endOffset) {
        RwChunkHeader header{};
        if (!reader.ReadHeader(header)) break;

        size_t nextChunk = reader.GetOffset() + header.size;
        if (header.type == rwID_BINMESH) {
            uint32_t flags = 0;
            uint32_t numMeshes = 0;
            uint32_t totalIndices = 0;

            if (reader.Read(flags) && reader.Read(numMeshes) && reader.Read(totalIndices)) {
                bool isTriStrip = (flags != 0);

                for (uint32_t m = 0; m < numMeshes; ++m) {
                    uint32_t numIndices = 0;
                    uint32_t matIndex = 0;
                    if (!reader.Read(numIndices) || !reader.Read(matIndex)) break;

                    DFFSubMesh subMesh;
                    if (matIndex < texNames.size()) {
                        subMesh.textureName = texNames[matIndex];
                    }

                    std::vector<uint32_t> rawIdx(numIndices);
                    for (uint32_t k = 0; k < numIndices; ++k) {
                        reader.Read(rawIdx[k]);
                    }

                    if (isTriStrip) {
                        // Unpack triangle strip to triangle list
                        for (size_t i = 0; i + 2 < rawIdx.size(); ++i) {
                            uint32_t a = rawIdx[i];
                            uint32_t b = rawIdx[i + 1];
                            uint32_t c = rawIdx[i + 2];

                            // Abaikan degenerate triangle
                            if (a == b || b == c || a == c) continue;

                            if (i % 2 == 0) {
                                subMesh.indices.push_back(a);
                                subMesh.indices.push_back(b);
                                subMesh.indices.push_back(c);
                            } else {
                                subMesh.indices.push_back(a);
                                subMesh.indices.push_back(c);
                                subMesh.indices.push_back(b);
                            }
                        }
                    } else {
                        subMesh.indices = std::move(rawIdx);
                    }

                    if (!subMesh.indices.empty()) {
                        outGeom.subMeshes.push_back(std::move(subMesh));
                    }
                }
                found = true;
            }
        }
        reader.SetOffset(nextChunk);
    }

    return found;
}

} // namespace samp_editor
