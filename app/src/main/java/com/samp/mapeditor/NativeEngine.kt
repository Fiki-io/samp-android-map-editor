package com.samp.mapeditor

object NativeEngine {
    init {
        System.loadLibrary("samp_map_editor")
    }

    external fun nativeInit(
        sampImgPath: String,
        sampIdePath: String,
        modelVert: String,
        modelFrag: String,
        gridVert: String,
        gridFrag: String
    ): Boolean

    external fun nativeSetAssetManager(assetManager: Any)

    external fun nativeLoadGTA3Fd(fd: Int, length: Long): Boolean
    external fun nativeLoadSAMPFd(fd: Int, length: Long): Boolean
    external fun nativeLoadIPL(iplPath: String): Boolean
    external fun nativeLoadArea(area: String, dataDir: String): Boolean
    external fun nativeClearWorld()
    external fun nativeSetCameraPos(x: Float, y: Float, z: Float, yaw: Float, pitch: Float)
    external fun nativeGetEngineStats(): String

    external fun nativeSurfaceChanged(width: Int, height: Int)
    external fun nativeRenderFrame()

    external fun nativeTouchDown(x: Float, y: Float): Boolean
    external fun nativeTouchMove(x: Float, y: Float)
    external fun nativeTouchUp()

    external fun nativeCameraMove(forward: Float, right: Float, up: Float, dt: Float)
    external fun nativeCameraRotate(deltaYaw: Float, deltaPitch: Float)
    external fun nativeCameraZoom(deltaDist: Float)

    external fun nativeSpawnObject(modelId: Int, x: Float, y: Float, z: Float): Int
    external fun nativeSpawnObjectInFrontOfCamera(modelId: Int, distance: Float): Int

    external fun nativeDeleteSelected()
    external fun nativeDuplicateSelected(): Int

    external fun nativeExportPawn(format: Int): String
    external fun nativeGetSelectedObjectInfo(): String
    external fun nativeUpdateSelectedObject(x: Float, y: Float, z: Float, rx: Float, ry: Float, rz: Float)
    external fun nativeSearchObjects(query: String, maxResults: Int): String
}
