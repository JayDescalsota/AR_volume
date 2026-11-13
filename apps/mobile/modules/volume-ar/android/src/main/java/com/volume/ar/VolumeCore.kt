package com.volume.ar

object VolumeCore {
  init {
    System.loadLibrary("volume_core_jni")
  }

  private val statusNames = listOf(
    "Ok",
    "DegeneratePolygon",
    "DegenerateHeight",
    "DegeneratePlane",
    "MismatchedLoops",
    "NonFinite",
  )

  private external fun nativeCompute(
    footprint: DoubleArray,
    mode: Int,
    manualHeight: Double,
    base: DoubleArray,
    top: DoubleArray,
    roof: DoubleArray,
    up: DoubleArray,
  ): DoubleArray

  private external fun nativePolygonArea(points: DoubleArray): Double

  fun polygonArea(points: DoubleArray): Double = nativePolygonArea(points)

  fun compute(
    footprint: DoubleArray,
    mode: String,
    manualHeight: Double,
    base: DoubleArray,
    top: DoubleArray,
    roof: DoubleArray,
    up: DoubleArray,
  ): Map<String, Any> {
    val modeCode = when (mode) {
      "taps" -> 1
      "roof" -> 2
      else -> 0
    }
    val values = nativeCompute(footprint, modeCode, manualHeight, base, top, roof, up)
    val statusIndex = values.getOrElse(6) { 0.0 }.toInt().coerceIn(0, statusNames.size - 1)
    return mapOf(
      "area" to values.getOrElse(0) { 0.0 },
      "perimeter" to values.getOrElse(1) { 0.0 },
      "height" to values.getOrElse(2) { 0.0 },
      "volume" to values.getOrElse(3) { 0.0 },
      "relativeUncertainty" to values.getOrElse(4) { 0.0 },
      "planar" to (values.getOrElse(5) { 0.0 } > 0.5),
      "status" to statusNames[statusIndex],
    )
  }
}
