#pragma once

#include "../core/types.h"
#include "../archive/img_archive.h"
#include "../format/dff_loader.h"
#include "../format/txd_loader.h"
#include "../format/ide_parser.h"
#include "../format/ipl_parser.h"
#include "exporter.h"
#include "spatial_grid.h"
#include "gizmo.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace samp_editor {

class Scene {
public:
    Scene();
    ~Scene();

    bool LoadSAMPAssets(const std::string& sampImgPath, const std::string& sampIdePath);
    bool LoadGTA3Archive(const std::string& gta3ImgPath);
    bool LoadGTA3ArchiveFd(int fd, uint64_t fileLength = 0);
    bool LoadIDEFile(const std::string& idePath);
    bool LoadIPLFile(const std::string& iplPath);
    bool LoadArea(const std::string& areaName, const std::string& dataDir);
    void ClearWorldInstances();

    size_t GetWorldInstanceCount() const { return m_WorldInstances.size(); }
    size_t GetObjectDefCount() const { return m_ObjectDefs.size(); }
    size_t GetGTA3EntryCount() const { return m_Gta3Archive.GetEntryCount(); }
    size_t GetSAMPEntryCount() const { return m_SampArchive.GetEntryCount(); }

    // Object placement & editing
    EditorObject* SpawnObject(uint32_t modelId, const Vec3& position);
    void RemoveObject(uint32_t objectId);
    EditorObject* GetObject(uint32_t objectId);
    void ClearObjects();

    // Raycast Picking
    EditorObject* PickObject(const Ray& ray, float& outHitDist);

    // Dynamic Spatial streaming
    void QueryVisibleObjects(const Vec3& camPos, float radius, std::vector<EditorObject*>& outEditorObjs, std::vector<const MapInstance*>& outWorldObjs);

    // Asset Retrieval & Lazy Loading
    const DFFModel* GetOrLoadModel(const std::string& modelName);
    const DecodedTexture* GetOrLoadTexture(const std::string& textureName, const std::string& txdNameHint = "");

    const std::unordered_map<uint32_t, ObjectDef>& GetObjectDefinitions() const { return m_ObjectDefs; }
    const std::vector<std::unique_ptr<EditorObject>>& GetEditorObjects() const { return m_EditorObjects; }
    TransformGizmo& GetGizmo() { return m_Gizmo; }

    bool HasGTA3Archive() const { return m_Gta3Archive.IsOpen(); }
    bool HasSAMPArchive() const { return m_SampArchive.IsOpen(); }

private:
    uint32_t m_NextObjectId{1};
    std::unordered_map<uint32_t, ObjectDef> m_ObjectDefs;

    IMGArchive m_Gta3Archive;
    IMGArchive m_SampArchive;

    std::vector<std::unique_ptr<EditorObject>> m_EditorObjects;
    std::vector<MapInstance> m_WorldInstances;
    SpatialGrid2D<size_t> m_WorldSpatialGrid;

    std::unordered_map<std::string, DFFModel> m_ModelCache;
    std::unordered_map<std::string, DecodedTexture> m_TextureCache;

    TransformGizmo m_Gizmo;
};

} // namespace samp_editor
