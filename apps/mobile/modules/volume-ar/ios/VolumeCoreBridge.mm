#import "VolumeCoreBridge.h"

#include <string>
#include <vector>

#include "volume/measurement.h"
#include "volume/plane.h"
#include "volume/polygon.h"

using namespace volume;

namespace {

std::vector<Vec3> toPoints(NSArray<NSNumber *> *values) {
  std::vector<Vec3> out;
  if (values == nil) return out;
  const NSUInteger count = values.count / 3;
  out.reserve(count);
  for (NSUInteger i = 0; i < count; ++i) {
    out.emplace_back(values[i * 3 + 0].doubleValue,
                     values[i * 3 + 1].doubleValue,
                     values[i * 3 + 2].doubleValue);
  }
  return out;
}

Vec3 firstOr(NSArray<NSNumber *> *values, Vec3 fallback) {
  if (values == nil || values.count < 3) return fallback;
  return Vec3{values[0].doubleValue, values[1].doubleValue, values[2].doubleValue};
}

NSString *statusName(SolidStatus status) {
  switch (status) {
    case SolidStatus::Ok: return @"Ok";
    case SolidStatus::DegeneratePolygon: return @"DegeneratePolygon";
    case SolidStatus::DegenerateHeight: return @"DegenerateHeight";
    case SolidStatus::DegeneratePlane: return @"DegeneratePlane";
    case SolidStatus::MismatchedLoops: return @"MismatchedLoops";
    case SolidStatus::NonFinite: return @"NonFinite";
  }
  return @"Unknown";
}

}  // namespace

@implementation VolumeCoreBridge

+ (NSDictionary *)computeWithFootprint:(NSArray<NSNumber *> *)footprint
                                  mode:(NSString *)mode
                          manualHeight:(double)manualHeight
                                  base:(NSArray<NSNumber *> *)base
                                   top:(NSArray<NSNumber *> *)top
                                  roof:(NSArray<NSNumber *> *)roof
                                    up:(NSArray<NSNumber *> *)up {
  HeightInput input;
  if ([mode isEqualToString:@"taps"]) {
    input.mode = HeightMode::Taps;
  } else if ([mode isEqualToString:@"roof"]) {
    input.mode = HeightMode::RoofPlane;
  } else {
    input.mode = HeightMode::Manual;
  }

  input.manualHeight = manualHeight;
  input.up = firstOr(up, Vec3{0.0, 1.0, 0.0});
  input.base = firstOr(base, Vec3{0.0, 0.0, 0.0});
  input.top = firstOr(top, Vec3{0.0, 0.0, 0.0});

  const std::vector<Vec3> roofPoints = toPoints(roof);
  if (roofPoints.size() >= 3) {
    input.roofPlane = planeFromLoop(roofPoints);
  }

  const MeasurementResult result = computeMeasurement(toPoints(footprint), input);

  return @{
    @"area" : @(result.area),
    @"perimeter" : @(result.perimeter),
    @"height" : @(result.height),
    @"volume" : @(result.volume),
    @"relativeUncertainty" : @(result.relativeUncertainty),
    @"planar" : @(result.planarFootprint),
    @"status" : statusName(result.status),
  };
}

+ (double)polygonArea:(NSArray<NSNumber *> *)points {
  return polygonArea(toPoints(points));
}

@end
