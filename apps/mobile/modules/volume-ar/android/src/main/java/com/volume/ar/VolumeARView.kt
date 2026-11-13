package com.volume.ar

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.opengl.Matrix
import android.util.Log
import androidx.core.content.ContextCompat
import com.google.ar.core.ArCoreApk
import com.google.ar.core.Config
import com.google.ar.core.Session
import com.google.ar.core.TrackingState
import expo.modules.kotlin.AppContext
import expo.modules.kotlin.views.ExpoView
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class VolumeARView(context: Context, appContext: AppContext) :
  ExpoView(context, appContext), GLSurfaceView.Renderer {

  private val surfaceView = GLSurfaceView(context)
  private val background = CameraBackgroundRenderer()
  private var session: Session? = null
  private var surfaceWidth = 1
  private var surfaceHeight = 1
  private var lastEmit = 0L

  var active: Boolean = true
    set(value) {
      field = value
      if (value) startSession() else stopSession()
    }

  init {
    surfaceView.setEGLContextClientVersion(2)
    surfaceView.preserveEGLContextOnPause = true
    surfaceView.setRenderer(this)
    surfaceView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
    addView(surfaceView)
  }

  override fun onAttachedToWindow() {
    super.onAttachedToWindow()
    if (active) startSession()
  }

  override fun onDetachedFromWindow() {
    stopSession()
    super.onDetachedFromWindow()
  }

  override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
    surfaceView.layout(0, 0, right - left, bottom - top)
  }

  private fun startSession() {
    session?.let {
      it.resume()
      surfaceView.onResume()
      return
    }

    if (ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) !=
      PackageManager.PERMISSION_GRANTED
    ) {
      appContext.currentActivity?.requestPermissions(
        arrayOf(Manifest.permission.CAMERA),
        PERMISSION_REQUEST,
      )
      return
    }

    try {
      val availability = ArCoreApk.getInstance().checkAvailability(context)
      if (!availability.isSupported && !availability.isTransient) {
        ARState.tracking = "unsupported"
        Log.e(TAG, "ARCore is not supported on this device: ${availability.name}")
        return
      }

      when (ArCoreApk.getInstance().requestInstall(appContext.currentActivity, true)) {
        ArCoreApk.InstallStatus.INSTALLED -> Unit
        ArCoreApk.InstallStatus.INSTALL_REQUESTED -> return
      }

      val created = Session(context)
      val config = Config(created).apply {
        planeFindingMode = Config.PlaneFindingMode.HORIZONTAL_AND_VERTICAL
        lightEstimationMode = Config.LightEstimationMode.ENVIRONMENTAL_HDR
        if (created.isDepthModeSupported(Config.DepthMode.AUTOMATIC)) {
          depthMode = Config.DepthMode.AUTOMATIC
        }
      }
      created.configure(config)
      created.resume()

      session = created
      ARState.session = created
      ARState.active = true
      surfaceView.onResume()
    } catch (error: Exception) {
      Log.e(TAG, "Unable to start ARCore session", error)
    }
  }

  private fun stopSession() {
    ARState.active = false
    session?.pause()
    surfaceView.onPause()
  }

  override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
    GLES20.glClearColor(0f, 0f, 0f, 1f)
    background.create()
  }

  override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
    surfaceWidth = width.coerceAtLeast(1)
    surfaceHeight = height.coerceAtLeast(1)
    GLES20.glViewport(0, 0, surfaceWidth, surfaceHeight)
    session?.setDisplayGeometry(0, surfaceWidth, surfaceHeight)
  }

  override fun onDrawFrame(gl: GL10?) {
    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT or GLES20.GL_DEPTH_BUFFER_BIT)

    val current = session ?: return
    current.setCameraTextureName(background.textureId)

    val frame = try {
      current.update()
    } catch (error: Exception) {
      Log.e(TAG, "ARCore update failed", error)
      return
    }

    ARState.latestFrame = frame
    background.draw()
    publishFrame(frame)
  }

  private fun publishFrame(frame: com.google.ar.core.Frame) {
    val projection = FloatArray(16)
    val viewMatrix = FloatArray(16)
    val viewProjection = FloatArray(16)

    frame.camera.getProjectionMatrix(projection, 0, 0.01f, 300f)
    frame.camera.getViewMatrix(viewMatrix, 0)
    Matrix.multiplyMM(viewProjection, 0, projection, 0, viewMatrix, 0)

    val rowMajor = DoubleArray(16)
    for (column in 0..3) {
      for (row in 0..3) {
        rowMajor[row * 4 + column] = viewProjection[column * 4 + row].toDouble()
      }
    }
    ARState.latestViewProjection = rowMajor
    ARState.tracking = when (frame.camera.trackingState) {
      TrackingState.TRACKING -> "normal"
      TrackingState.PAUSED -> "limited"
      TrackingState.STOPPED -> "stopped"
    }

    val now = System.currentTimeMillis()
    if (now - lastEmit < 100) return
    lastEmit = now

    ARState.onFrame?.invoke(
      mapOf(
        "viewProjection" to rowMajor.toList(),
        "tracking" to ARState.tracking,
        "width" to surfaceWidth.toDouble(),
        "height" to surfaceHeight.toDouble(),
      ),
    )
  }

  companion object {
    private const val TAG = "VolumeAR"
    const val PERMISSION_REQUEST = 9001
  }
}
