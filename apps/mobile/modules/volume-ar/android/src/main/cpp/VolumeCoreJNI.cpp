#include <jni.h>

#include <vector>

#include "volume/measurement.h"
#include "volume/plane.h"
#include "volume/polygon.h"

using namespace volume;

namespace {

std::vector<Vec3> toPoints(JNIEnv* env, jdoubleArray array) {
  std::vector<Vec3> points;
  if (array == nullptr) return points;
  const jsize length = env->GetArrayLength(array);
  if (length < 3) return points;
  std::vector<jdouble> buffer(static_cast<size_t>(length));
  env->GetDoubleArrayRegion(array, 0, length, buffer.data());
  for (jsize i = 0; i + 2 < length; i += 3) {
    points.emplace_back(buffer[i], buffer[i + 1], buffer[i + 2]);
  }
  return points;
}

}  // namespace

extern "C" JNIEXPORT jdoubleArray JNICALL
Java_com_volume_ar_VolumeCore_nativeCompute(
    JNIEnv* env, jobject /*thiz*/, jdoubleArray footprint, jint mode,
    jdouble manualHeight, jdoubleArray base, jdoubleArray top,
    jdoubleArray roof, jdoubleArray up) {
  HeightInput input;
  if (mode == 1) {
    input.mode = HeightMode::Taps;
  } else if (mode == 2) {
    input.mode = HeightMode::RoofPlane;
  } else {
    input.mode = HeightMode::Manual;
  }
  input.manualHeight = manualHeight;

  const auto upPoints = toPoints(env, up);
  if (!upPoints.empty()) input.up = upPoints[0];

  const auto basePoints = toPoints(env, base);
  if (!basePoints.empty()) input.base = basePoints[0];

  const auto topPoints = toPoints(env, top);
  if (!topPoints.empty()) input.top = topPoints[0];

  const auto roofPoints = toPoints(env, roof);
  if (roofPoints.size() >= 3) input.roofPlane = planeFromLoop(roofPoints);

  const MeasurementResult result = computeMeasurement(toPoints(env, footprint), input);

  jdouble values[7] = {
      result.area,
      result.perimeter,
      result.height,
      result.volume,
      result.relativeUncertainty,
      result.planarFootprint ? 1.0 : 0.0,
      static_cast<jdouble>(static_cast<int>(result.status)),
  };

  jdoubleArray out = env->NewDoubleArray(7);
  env->SetDoubleArrayRegion(out, 0, 7, values);
  return out;
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_volume_ar_VolumeCore_nativePolygonArea(JNIEnv* env, jobject /*thiz*/,
                                                jdoubleArray points) {
  return polygonArea(toPoints(env, points));
}
