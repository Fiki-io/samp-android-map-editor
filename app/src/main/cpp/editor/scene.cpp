#include "scene.h"
#include <iostream>
#include <algorithm>

namespace samp_editor {

namespace {
    std::string ToLower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }
}

Scene::Scene() : m_WorldSpatialGrid(-3000.0f, 3000.0f, 250.0f) {}

Scene::~Scene() {
    ClearObjects();
}

bool Scene::LoadSAMPAssets(const std::string& sampImgPath, const std::string& sampIdePath) {
    bool okImg = m_SampArchive.Open(sampImgPath);
    if (!okImg) {
        std::cerr << "[Scene] Failed to open SAMP.img: " << sampImgPath << std::endl;
    }

    bool okIde = IDEParser::ParseFile(sampIdePath, m_ObjectDefs);
    if (!okIde) {
        std::cerr << "[Scene] Failed to parse SAMP.ide: " << sampIdePath << std::endl;
    }

    return okImg || okIde;
}

bool Scene::LoadGTA3Archive(const std::string& gta3ImgPath) {
    return m_Gta3Archive.Open(gta3ImgPath);
}

bool Scene::LoadGTA3ArchiveFd(int fd, uint64_t fileLength) {
    return m_Gta3Archive.OpenFromFd(fd, fileLength);
}

bool Scene::LoadIDEFile(const std::string& idePath) {
    return IDEParser::ParseFile(idePath, m_ObjectDefs);
}

bool Scene::LoadIPLFile(const std::string& iplPath) {
    std::vector<MapInstance> instances;
    if (!IPLParser::ParseFile(iplPath, instances)) return false;

    size_t baseIdx = m_WorldInstances.size();
    for (size_t i = 0; i < instances.size(); ++i) {
        m_WorldSpatialGrid.Insert(instances[i].position, baseIdx + i);
        m_WorldInstances.push_back(std::move(instances[i]));
    }
    return true;
}

void Scene::ClearWorldInstances() {
    m_WorldInstances.clear();
    m_WorldSpatialGrid.Clear();
}

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/log.h>
#define SCENE_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Scene", __VA_ARGS__)
#define SCENE_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Scene", __VA_ARGS__)
#else
#define SCENE_LOGI(...)
#define SCENE_LOGE(...)
#endif

static std::string ReadAssetContent(void* assetManager, const std::string& relativePath) {
#ifdef __ANDROID__
    if (!assetManager) return "";
    AAssetManager* mgr = static_cast<AAssetManager*>(assetManager);
    AAsset* asset = AAssetManager_open(mgr, relativePath.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        SCENE_LOGE("Failed to open asset: %s", relativePath.c_str());
        return "";
    }
    size_t size = AAsset_getLength(asset);
    std::string content;
    content.resize(size);
    int readBytes = AAsset_read(asset, &content[0], size);
    AAsset_close(asset);
    if (readBytes < 0) {
        SCENE_LOGE("Failed to read asset: %s", relativePath.c_str());
        return "";
    }
    return content;
#else
    return "";
#endif
}

bool Scene::LoadArea(const std::string& areaName, const std::string& dataDir) {
    // Load generic definitions
    LoadIDEFile(dataDir + "/maps/generic/barriers.ide");
    LoadIDEFile(dataDir + "/maps/generic/dynamic.ide");
    LoadIDEFile(dataDir + "/maps/generic/dynamic2.ide");
    LoadIDEFile(dataDir + "/maps/generic/vegepart.ide");

    if (areaName == "LA" || areaName == "all") {
        std::vector<std::string> laFiles = {
            "LAe", "LAe2", "LAn", "LAn2", "LAs", "LAs2", "LAw", "LAw2", "LaWn", "LAhills"
        };
        for (const auto& f : laFiles) {
            LoadIDEFile(dataDir + "/maps/LA/" + f + ".ide");
            LoadIPLFile(dataDir + "/maps/LA/" + f + ".ipl");
        }
    }
    return !m_WorldInstances.empty();
}

bool Scene::LoadAreaFromAssets(void* aAssetManager, const std::string& areaName) {
#ifdef __ANDROID__
    if (!aAssetManager) return false;

    // 1. Generic IDE definitions
    std::vector<std::string> genericIdes = {
        "data/maps/generic/barriers.ide",
        "data/maps/generic/dynamic.ide",
        "data/maps/generic/dynamic2.ide",
        "data/maps/generic/vegepart.ide"
    };
    for (const auto& path : genericIdes) {
        std::string content = ReadAssetContent(aAssetManager, path);
        if (!content.empty()) {
            IDEParser::ParseString(content, m_ObjectDefs);
        }
    }

    // 2. LA area map instances and definitions
    if (areaName == "LA" || areaName == "all") {
        std::vector<std::string> laFiles = {
            "LAe", "LAe2", "LAn", "LAn2", "LAs", "LAs2", "LAw", "LAw2", "LaWn", "LAhills"
        };
        for (const auto& f : laFiles) {
            std::string idePath = "data/maps/LA/" + f + ".ide";
            std::string iplPath = "data/maps/LA/" + f + ".ipl";

            std::string ideContent = ReadAssetContent(aAssetManager, idePath);
            if (!ideContent.empty()) {
                IDEParser::ParseString(ideContent, m_ObjectDefs);
            }

            std::string iplContent = ReadAssetContent(aAssetManager, iplPath);
            if (!iplContent.empty()) {
                std::vector<MapInstance> instances;
                if (IPLParser::ParseString(iplContent, instances)) {
                    size_t baseIdx = m_WorldInstances.size();
                    for (size_t i = 0; i < instances.size(); ++i) {
                        m_WorldSpatialGrid.Insert(instances[i].position, baseIdx + i);
                        m_WorldInstances.push_back(std::move(instances[i]));
                    }
                }
            }
        }
    }
    SCENE_LOGI("LoadAreaFromAssets %s complete: loaded %zu world instances, %zu total object defs",
               areaName.c_str(), m_WorldInstances.size(), m_ObjectDefs.size());
    return !m_WorldInstances.empty();
#else
    return false;
#endif
}

EditorObject* Scene::SpawnObject(uint32_t modelId, const Vec3& position) {
    auto obj = std::make_unique<EditorObject>();
    obj->id = m_NextObjectId++;
    obj->modelId = modelId;
    obj->position = position;
    obj->rotationQuat = Quat::Identity();
    obj->rotationEuler = {0, 0, 0};

    auto it = m_ObjectDefs.find(modelId);
    if (it != m_ObjectDefs.end()) {
        obj->name = it->second.modelName;
        obj->drawDistance = it->second.drawDistance;
    } else {
        obj->name = "Object_" + std::to_string(modelId);
    }

    EditorObject* rawPtr = obj.get();
    m_EditorObjects.push_back(std::move(obj));
    m_Gizmo.SetTarget(rawPtr);
    return rawPtr;
}

void Scene::RemoveObject(uint32_t objectId) {
    if (m_Gizmo.GetTarget() && m_Gizmo.GetTarget()->id == objectId) {
        m_Gizmo.SetTarget(nullptr);
    }

    auto it = std::remove_if(m_EditorObjects.begin(), m_EditorObjects.end(),
        [objectId](const std::unique_ptr<EditorObject>& obj) { return obj && obj->id == objectId; });
    m_EditorObjects.erase(it, m_EditorObjects.end());
}

EditorObject* Scene::GetObject(uint32_t objectId) {
    for (auto& obj : m_EditorObjects) {
        if (obj && obj->id == objectId) return obj.get();
    }
    return nullptr;
}

void Scene::ClearObjects() {
    m_Gizmo.SetTarget(nullptr);
    m_EditorObjects.clear();
}

EditorObject* Scene::PickObject(const Ray& ray, float& outHitDist) {
    EditorObject* bestObj = nullptr;
    float closestDist = 1e9f;

    for (auto& objPtr : m_EditorObjects) {
        if (!objPtr) continue;
        auto& obj = *objPtr;

        const DFFModel* model = GetOrLoadModel(obj.name);
        AABB worldBox;
        if (model && model->isValid) {
            worldBox.min = obj.position + model->boundingBox.min;
            worldBox.max = obj.position + model->boundingBox.max;
        } else {
            worldBox.min = obj.position - Vec3(1.0f, 1.0f, 1.0f);
            worldBox.max = obj.position + Vec3(1.0f, 1.0f, 1.0f);
        }

        float tMin, tMax;
        if (ray.IntersectAABB(worldBox, tMin, tMax)) {
            if (tMin < closestDist) {
                closestDist = tMin;
                bestObj = &obj;
            }
        }
    }

    if (bestObj) {
        outHitDist = closestDist;
        m_Gizmo.SetTarget(bestObj);
    }
    return bestObj;
}

void Scene::QueryVisibleObjects(const Vec3& camPos, float radius,
                               std::vector<EditorObject*>& outEditorObjs,
                               std::vector<const MapInstance*>& outWorldObjs) {
    float rSq = radius * radius;

    for (auto& obj : m_EditorObjects) {
        if (obj && (obj->position - camPos).LengthSq() <= rSq) {
            outEditorObjs.push_back(obj.get());
        }
    }

    // 2. World background instances (melalui spatial hash grid)
    std::vector<size_t> candidateIndices;
    m_WorldSpatialGrid.QueryRadius(camPos, radius, candidateIndices);

    for (size_t idx : candidateIndices) {
        if (idx < m_WorldInstances.size()) {
            const auto& inst = m_WorldInstances[idx];
            if ((inst.position - camPos).LengthSq() <= rSq) {
                outWorldObjs.push_back(&inst);
            }
        }
    }
}

const DFFModel* Scene::GetOrLoadModel(const std::string& modelName) {
    std::string key = ToLower(modelName);
    if (key.empty()) return nullptr;

    auto it = m_ModelCache.find(key);
    if (it != m_ModelCache.end()) {
        return &it->second;
    }

    std::string dffFileName = key + ".dff";
    std::vector<uint8_t> buffer;

    // Prioritaskan SAMP.img dulu, baru gta3.img
    bool readOk = false;
    if (m_SampArchive.IsOpen() && m_SampArchive.HasEntry(dffFileName)) {
        readOk = m_SampArchive.ReadEntry(dffFileName, buffer);
    }
    if (!readOk && m_Gta3Archive.IsOpen() && m_Gta3Archive.HasEntry(dffFileName)) {
        readOk = m_Gta3Archive.ReadEntry(dffFileName, buffer);
    }

    if (readOk) {
        DFFModel model;
        if (DFFLoader::LoadFromMemory(buffer.data(), buffer.size(), model, key)) {
            m_ModelCache[key] = std::move(model);
            return &m_ModelCache[key];
        }
    }

    return nullptr;
}

const DecodedTexture* Scene::GetOrLoadTexture(const std::string& textureName, const std::string& txdNameHint) {
    std::string key = ToLower(textureName);
    auto it = m_TextureCache.find(key);
    if (it != m_TextureCache.end()) {
        return &it->second;
    }

    std::vector<std::string> txdCandidates;
    if (!txdNameHint.empty()) {
        txdCandidates.push_back(ToLower(txdNameHint) + ".txd");
    }
    txdCandidates.push_back(key + ".txd");

    for (const auto& txdFile : txdCandidates) {
        std::vector<uint8_t> buffer;
        bool readOk = false;
        if (m_SampArchive.IsOpen() && m_SampArchive.HasEntry(txdFile)) {
            readOk = m_SampArchive.ReadEntry(txdFile, buffer);
        }
        if (!readOk && m_Gta3Archive.IsOpen() && m_Gta3Archive.HasEntry(txdFile)) {
            readOk = m_Gta3Archive.ReadEntry(txdFile, buffer);
        }

        if (readOk) {
            std::unordered_map<std::string, DecodedTexture> loaded;
            if (TXDLoader::LoadFromMemory(buffer.data(), buffer.size(), loaded)) {
                for (auto& [name, tex] : loaded) {
                    m_TextureCache[name] = std::move(tex);
                }
                auto found = m_TextureCache.find(key);
                if (found != m_TextureCache.end()) {
                    return &found->second;
                }
            }
        }
    }

    return nullptr;
}

} // namespace samp_editor
