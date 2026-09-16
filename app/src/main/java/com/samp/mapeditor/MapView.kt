package com.samp.mapeditor

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10
import kotlin.math.sqrt

class MapView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : GLSurfaceView(context, attrs), GLSurfaceView.Renderer {

    var onObjectSelectedListener: ((String) -> Unit)? = null

    private var previousX = 0f
    private var previousY = 0f
    private var isDraggingGizmo = false

    private val scaleDetector = ScaleGestureDetector(context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScale(detector: ScaleGestureDetector): Boolean {
            val scaleFactor = detector.scaleFactor
            val delta = (scaleFactor - 1.0f) * 20.0f
            NativeEngine.nativeCameraZoom(delta)
            return true
        }
    })

    init {
        setEGLContextClientVersion(3)
        setRenderer(this)
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        // Shaders & assets diinisialisasi via MainActivity saat startup
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        NativeEngine.nativeSurfaceChanged(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        NativeEngine.nativeRenderFrame()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        scaleDetector.onTouchEvent(event)

        if (event.pointerCount > 1) {
            // Mode multi-touch untuk zoom / pan
            isDraggingGizmo = false
            NativeEngine.nativeTouchUp()
            return true
        }

        val x = event.x
        val y = event.y

        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                previousX = x
                previousY = y
                isDraggingGizmo = NativeEngine.nativeTouchDown(x, y)

                // Cek apakah ada objek yang terpilih
                val info = NativeEngine.nativeGetSelectedObjectInfo()
                onObjectSelectedListener?.invoke(info)
            }
            MotionEvent.ACTION_MOVE -> {
                val dx = x - previousX
                val dy = y - previousY

                if (isDraggingGizmo) {
                    NativeEngine.nativeTouchMove(x, y)
                    val info = NativeEngine.nativeGetSelectedObjectInfo()
                    onObjectSelectedListener?.invoke(info)
                } else {
                    // Kamera Flycam Rotate (Yaw & Pitch)
                    NativeEngine.nativeCameraRotate(-dx * 0.25f, -dy * 0.25f)
                }

                previousX = x
                previousY = y
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                isDraggingGizmo = false
                NativeEngine.nativeTouchUp()
            }
        }
        return true
    }
}
