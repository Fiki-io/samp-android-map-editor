#include <jni.h>
#include <string>
#include <sstream>
#include <memory>
#include <android/log.h>
#include "editor/scene.h"
#include "editor/raycast.h"
#include "renderer/gl_renderer.h"

#define LOG_TAG "NativeEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace samp_editor;

static std::unique_ptr<Scene> g_Scene;
static std::unique_ptr<GLRenderer> g_Renderer;

static float g_LastTouchX = 0.0f;
static float g_LastTouchY = 0.0f;
static bool g_IsTouching = false;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeInit(
    JNIEnv* env, jobject /* this */,
    jstring jSampImg, jstring jSampIde,
    jstring jModelVert, jstring jModelFrag,
    jstring jGridVert, jstring jGridFrag) {

    const char* sampImg = env->GetStringUTFChars(jSampImg, nullptr);
    const char* sampIde = env->GetStringUTFChars(jSampIde, nullptr);
    const char* modelVert = env->GetStringUTFChars(jModelVert, nullptr);
    const char* modelFrag = env->GetStringUTFChars(jModelFrag, nullptr);
    const char* gridVert = env->GetStringUTFChars(jGridVert, nullptr);
    const char* gridFrag = env->GetStringUTFChars(jGridFrag, nullptr);

    g_Scene = std::make_unique<Scene>();
    g_Renderer = std::make_unique<GLRenderer>();

    bool okAssets = g_Scene->LoadSAMPAssets(sampImg, sampIde);
    bool okShaders = g_Renderer->InitShaders(modelVert, modelFrag, gridVert, gridFrag);

    env->ReleaseStringUTFChars(jSampImg, sampImg);
    env->ReleaseStringUTFChars(jSampIde, sampIde);
    env->ReleaseStringUTFChars(jModelVert, modelVert);
    env->ReleaseStringUTFChars(jModelFrag, modelFrag);
    env->ReleaseStringUTFChars(jGridVert, gridVert);
    env->ReleaseStringUTFChars(jGridFrag, gridFrag);

    LOGI("[NativeEngine] Engine Initialized: Assets=%d, Shaders=%d", okAssets, okShaders);
    return okAssets && okShaders;
}

JNIEXPORT jboolean JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeLoadGTA3Fd(
    JNIEnv* /* env */, jobject /* this */, jint fd, jlong length) {
    if (!g_Scene) return JNI_FALSE;
    bool ok = g_Scene->LoadGTA3ArchiveFd(fd, static_cast<uint64_t>(length));
    LOGI("[NativeEngine] Load GTA3 from FD=%d, length=%lld: %d", fd, (long long)length, ok);
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeLoadIPL(
    JNIEnv* env, jobject /* this */, jstring jPath) {
    if (!g_Scene) return JNI_FALSE;
    const char* path = env->GetStringUTFChars(jPath, nullptr);
    bool ok = g_Scene->LoadIPLFile(path);
    env->ReleaseStringUTFChars(jPath, path);
    return ok ? JNI_TRUE : JNI_FALSE;
}

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

static AAssetManager* g_AssetManager = nullptr;

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeSetAssetManager(
    JNIEnv* env, jobject /* this */, jobject jAssetManager) {
    if (jAssetManager) {
        g_AssetManager = AAssetManager_fromJava(env, jAssetManager);
        LOGI("[NativeEngine] AssetManager registered successfully: %p", g_AssetManager);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeLoadArea(
    JNIEnv* env, jobject /* this */, jstring jArea, jstring jDataDir) {
    if (!g_Scene) return JNI_FALSE;
    const char* area = env->GetStringUTFChars(jArea, nullptr);
    const char* dataDir = env->GetStringUTFChars(jDataDir, nullptr);

    bool ok = false;
    if (g_AssetManager) {
        ok = g_Scene->LoadAreaFromAssets(g_AssetManager, area);
    }
    if (!ok && dataDir && strlen(dataDir) > 0) {
        ok = g_Scene->LoadArea(area, dataDir);
    }

    LOGI("[NativeEngine] nativeLoadArea %s (assetMgr=%p): instances=%zu, ok=%d", area, g_AssetManager, g_Scene->GetWorldInstanceCount(), ok);
    env->ReleaseStringUTFChars(jArea, area);
    env->ReleaseStringUTFChars(jDataDir, dataDir);
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeClearWorld(
    JNIEnv* /* env */, jobject /* this */) {
    if (g_Scene) {
        g_Scene->ClearWorldInstances();
    }
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeSetCameraPos(
    JNIEnv* /* env */, jobject /* this */, jfloat x, jfloat y, jfloat z, jfloat yaw, jfloat pitch) {
    if (g_Renderer) {
        g_Renderer->GetCamera().SetPosition({x, y, z});
        g_Renderer->GetCamera().SetRotation(yaw, pitch);
    }
}

JNIEXPORT jstring JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeGetEngineStats(
    JNIEnv* env, jobject /* this */) {
    if (!g_Scene || !g_Renderer) return env->NewStringUTF("{}");

    Vec3 pos = g_Renderer->GetCamera().GetPosition();
    std::stringstream ss;
    ss << "{"
       << "\"gta3Entries\":" << g_Scene->GetGTA3EntryCount()
       << ",\"sampEntries\":" << g_Scene->GetSAMPEntryCount()
       << ",\"defsCount\":" << g_Scene->GetObjectDefCount()
       << ",\"worldInstances\":" << g_Scene->GetWorldInstanceCount()
       << ",\"editorCount\":" << g_Scene->GetEditorObjects().size()
       << ",\"camX\":" << pos.x
       << ",\"camY\":" << pos.y
       << ",\"camZ\":" << pos.z
       << "}";
    return env->NewStringUTF(ss.str().c_str());
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeSurfaceChanged(
    JNIEnv* /* env */, jobject /* this */, jint width, jint height) {
    if (g_Renderer) {
        g_Renderer->OnSurfaceChanged(width, height);
    }
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeRenderFrame(
    JNIEnv* /* env */, jobject /* this */) {
    if (g_Renderer && g_Scene) {
        g_Renderer->Render(*g_Scene);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeTouchDown(
    JNIEnv* /* env */, jobject /* this */, jfloat x, jfloat y) {
    if (!g_Renderer || !g_Scene) return JNI_FALSE;

    g_LastTouchX = x;
    g_LastTouchY = y;
    g_IsTouching = true;

    float aspect = static_cast<float>(g_Renderer->GetWidth()) / static_cast<float>(g_Renderer->GetHeight());
    Mat4 view = g_Renderer->GetCamera().GetViewMatrix();
    Mat4 proj = g_Renderer->GetCamera().GetProjectionMatrix(aspect);

    Ray ray = RaycastUtil::ScreenPointToRay(x, y, g_Renderer->GetWidth(), g_Renderer->GetHeight(), view, proj);

    // 1. Cek apakah menyentuh Gizmo sumbu
    if (g_Scene->GetGizmo().HasTarget()) {
        GizmoAxis axis = g_Scene->GetGizmo().TestHit(ray);
        if (axis != GizmoAxis::None) {
            Vec3 hitPt = g_Scene->GetGizmo().GetPosition();
            g_Scene->GetGizmo().BeginDrag(axis, hitPt);
            return JNI_TRUE;
        }
    }

    // 2. Cek apakah menyentuh objek di scene
    float hitDist = 0.0f;
    EditorObject* picked = g_Scene->PickObject(ray, hitDist);
    return (picked != nullptr) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeTouchMove(
    JNIEnv* /* env */, jobject /* this */, jfloat x, jfloat y) {
    if (!g_Renderer || !g_Scene || !g_IsTouching) return;

    float aspect = static_cast<float>(g_Renderer->GetWidth()) / static_cast<float>(g_Renderer->GetHeight());
    Mat4 view = g_Renderer->GetCamera().GetViewMatrix();
    Mat4 proj = g_Renderer->GetCamera().GetProjectionMatrix(aspect);

    Ray ray = RaycastUtil::ScreenPointToRay(x, y, g_Renderer->GetWidth(), g_Renderer->GetHeight(), view, proj);

    if (g_Scene->GetGizmo().IsDragging()) {
        g_Scene->GetGizmo().UpdateDrag(ray);
    }

    g_LastTouchX = x;
    g_LastTouchY = y;
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeTouchUp(
    JNIEnv* /* env */, jobject /* this */) {
    if (g_Scene) {
        g_Scene->GetGizmo().EndDrag();
    }
    g_IsTouching = false;
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeCameraMove(
    JNIEnv* /* env */, jobject /* this */, jfloat forward, jfloat right, jfloat up, jfloat dt) {
    if (g_Renderer) {
        g_Renderer->GetCamera().Move({forward, right, up}, dt);
    }
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeCameraRotate(
    JNIEnv* /* env */, jobject /* this */, jfloat deltaYaw, jfloat deltaPitch) {
    if (g_Renderer) {
        g_Renderer->GetCamera().Rotate(deltaYaw, deltaPitch);
    }
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeCameraZoom(
    JNIEnv* /* env */, jobject /* this */, jfloat deltaDist) {
    if (g_Renderer) {
        g_Renderer->GetCamera().Zoom(deltaDist);
    }
}

JNIEXPORT jint JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeSpawnObject(
    JNIEnv* /* env */, jobject /* this */, jint modelId, jfloat x, jfloat y, jfloat z) {
    if (!g_Scene) return -1;
    EditorObject* obj = g_Scene->SpawnObject(static_cast<uint32_t>(modelId), {x, y, z});
    return obj ? static_cast<jint>(obj->id) : -1;
}

JNIEXPORT jint JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeSpawnObjectInFrontOfCamera(
    JNIEnv* /* env */, jobject /* this */, jint modelId, jfloat distance) {
    if (!g_Scene || !g_Renderer) return -1;
    Vec3 spawnPos = g_Renderer->GetCamera().GetPosition() + (g_Renderer->GetCamera().GetForward() * distance);
    EditorObject* obj = g_Scene->SpawnObject(static_cast<uint32_t>(modelId), spawnPos);
    return obj ? static_cast<jint>(obj->id) : -1;
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeDeleteSelected(
    JNIEnv* /* env */, jobject /* this */) {
    if (g_Scene && g_Scene->GetGizmo().HasTarget()) {
        g_Scene->RemoveObject(g_Scene->GetGizmo().GetTarget()->id);
    }
}

JNIEXPORT jint JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeDuplicateSelected(
    JNIEnv* /* env */, jobject /* this */) {
    if (!g_Scene || !g_Scene->GetGizmo().HasTarget()) return -1;
    const EditorObject* cur = g_Scene->GetGizmo().GetTarget();
    EditorObject* copy = g_Scene->SpawnObject(cur->modelId, cur->position + Vec3(1.0f, 1.0f, 0.0f));
    if (copy) {
        copy->rotationEuler = cur->rotationEuler;
        copy->SyncQuatFromEuler();
        return static_cast<jint>(copy->id);
    }
    return -1;
}

JNIEXPORT jstring JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeExportPawn(
    JNIEnv* env, jobject /* this */, jint format) {
    if (!g_Scene) return env->NewStringUTF("");
    ExportFormat fmt = (format == 1) ? ExportFormat::CreateObject_Standard : ExportFormat::CreateDynamicObject_Streamer;
    std::string pawnCode = PawnExporter::ExportToString(g_Scene->GetEditorObjects(), fmt);
    return env->NewStringUTF(pawnCode.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeGetSelectedObjectInfo(
    JNIEnv* env, jobject /* this */) {
    if (!g_Scene || !g_Scene->GetGizmo().HasTarget()) {
        return env->NewStringUTF("");
    }
    const auto* obj = g_Scene->GetGizmo().GetTarget();
    std::stringstream ss;
    ss << "{\"id\":" << obj->id
       << ",\"modelId\":" << obj->modelId
       << ",\"name\":\"" << obj->name << "\""
       << ",\"x\":" << obj->position.x
       << ",\"y\":" << obj->position.y
       << ",\"z\":" << obj->position.z
       << ",\"rx\":" << obj->rotationEuler.x
       << ",\"ry\":" << obj->rotationEuler.y
       << ",\"rz\":" << obj->rotationEuler.z
       << "}";
    return env->NewStringUTF(ss.str().c_str());
}

JNIEXPORT void JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeUpdateSelectedObject(
    JNIEnv* /* env */, jobject /* this */,
    jfloat x, jfloat y, jfloat z, jfloat rx, jfloat ry, jfloat rz) {
    if (g_Scene && g_Scene->GetGizmo().HasTarget()) {
        auto* obj = g_Scene->GetGizmo().GetTarget();
        obj->position = {x, y, z};
        obj->rotationEuler = {rx, ry, rz};
        obj->SyncQuatFromEuler();
    }
}

JNIEXPORT jstring JNICALL
Java_com_samp_mapeditor_NativeEngine_nativeSearchObjects(
    JNIEnv* env, jobject /* this */, jstring jQuery, jint maxResults) {
    if (!g_Scene) return env->NewStringUTF("[]");

    const char* queryC = env->GetStringUTFChars(jQuery, nullptr);
    std::string query(queryC);
    env->ReleaseStringUTFChars(jQuery, queryC);

    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    std::stringstream ss;
    ss << "[";
    int count = 0;

    for (const auto& [id, def] : g_Scene->GetObjectDefinitions()) {
        std::string lowerName = def.modelName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::string idStr = std::to_string(id);

        if (query.empty() || lowerName.find(query) != std::string::npos || idStr.find(query) != std::string::npos) {
            if (count > 0) ss << ",";
            ss << "{\"id\":" << id << ",\"name\":\"" << def.modelName << "\",\"txd\":\"" << def.txdName << "\"}";
            count++;
            if (count >= maxResults) break;
        }
    }
    ss << "]";
    return env->NewStringUTF(ss.str().c_str());
}

} // extern "C"
