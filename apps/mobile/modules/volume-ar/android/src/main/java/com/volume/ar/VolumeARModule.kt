package com.volume.ar

import android.content.Context
import com.google.ar.core.ArCoreApk
import com.google.ar.core.DepthPoint
import com.google.ar.core.Plane
import com.google.ar.core.Point
import com.google.ar.core.TrackingState
import expo.modules.kotlin.modules.Module
import expo.modules.kotlin.modules.ModuleDefinition

class VolumeARModule(context: Context) : Module() {
  private var cornerCounter = 0

  override fun definition() = ModuleDefinition {
    Name("VolumeAR")

    Events("onFrame", "onCornerUpdate")

    OnCreate {
      ARState.onFrame = { payload -> sendEvent("onFrame", payload) }
    }

    OnDestroy {
      ARState.onFrame = null
      ARState.session?.pause()
    }

    AsyncFunction("start") {
      val context = appContext.reactContext ?: throw Exception("No Android context available")
      val availability = ArCoreApk.getInstance().checkAvailability(context)
      if (!availability.isSupported && !availability.isTransient) {
        throw Exception(
          "ARCore is not supported on this device (${availability.name}). " +
            "Volume measurement needs an ARCore-certified phone.",
        )
      }
      ARState.active = true
      ARState.session?.resume()
    }

    AsyncFunction("stop") {
      ARState.active = false
      ARState.session?.pause()
    }

    Function("reset") {
      cornerCounter = 0
      ARState.clearAnchors()
    }

    AsyncFunction("addCorner") { px: Double, py: Double ->
      pick(px, py, true)
    }

    AsyncFunction("pickPoint") { px: Double, py: Double ->
      pick(px, py, false)
    }

    AsyncFunction("getViewProjection") {
      ARState.latestViewProjection.toList()
    }

    Function("polygonArea") { points: List<Double> ->
      VolumeCore.polygonArea(points.toDoubleArray())
    }

    Function("computeMeasurement") {
      footprint: List<Double>, mode: String, manualHeight: Double, base: List<Double>,
      top: List<Double>, roof: List<Double>, up: List<Double> ->
      VolumeCore.compute(
        footprint.toDoubleArray(),
        mode,
        manualHeight,
        base.toDoubleArray(),
        top.toDoubleArray(),
        roof.toDoubleArray(),
        up.toDoubleArray(),
      )
    }

    View(VolumeARView::class) {
      Prop("active") { view: VolumeARView, active: Boolean ->
        view.active = active
      }
    }
  }

  private fun pick(px: Double, py: Double, corner: Boolean): Map<String, Any> {
    val blank = mapOf(
      "id" to "",
      "x" to 0.0,
      "y" to 0.0,
      "z" to 0.0,
      "confidence" to 0.0,
      "rayCount" to 0,
    )

    val frame = ARState.latestFrame ?: return blank
    if (frame.camera.trackingState != TrackingState.TRACKING) return blank

    val density = appContext.reactContext?.resources?.displayMetrics?.density ?: 1f
    val screenX = (px * density).toFloat()
    val screenY = (py * density).toFloat()

    val hits = frame.hitTest(screenX, screenY)
    val hit = hits.firstOrNull { result ->
      when (val trackable = result.trackable) {
        is Plane -> trackable.isPoseInPolygon(result.hitPose)
        is DepthPoint -> trackable.confidence > 0.5f
        is Point -> true
        else -> false
      }
    }

    var x = 0.0
    var y = 0.0
    var z = 0.0
    var confidence = 0.2
    var rayCount = 1

    if (hit != null) {
      val position = hit.hitPose.translation
      x = position[0].toDouble()
      y = position[1].toDouble()
      z = position[2].toDouble()
      confidence = when (hit.trackable) {
        is Plane -> 0.95
        is DepthPoint -> 0.85
        is Point -> 0.6
        else -> 0.4
      }
      rayCount = if (confidence > 0.6) 3 else 1

      if (corner) {
        val anchor = ARState.session?.createAnchor(hit.hitPose)
        if (anchor != null) ARState.anchors.add(anchor)
      }
    } else {
      val cameraPose = frame.camera.pose
      val origin = cameraPose.translation
      val forward = floatArrayOf(0f, 0f, -1f)
      cameraPose.transformPoint(forward)
      val distance = 5.0f
      x = (origin[0] + forward[0] * distance).toDouble()
      y = (origin[1] + forward[1] * distance).toDouble()
      z = (origin[2] + forward[2] * distance).toDouble()
    }

    val id = if (corner) {
      cornerCounter += 1
      "c$cornerCounter"
    } else {
      "p${System.nanoTime()}"
    }

    return mapOf(
      "id" to id,
      "x" to x,
      "y" to y,
      "z" to z,
      "confidence" to confidence,
      "rayCount" to rayCount,
    )
  }
}
