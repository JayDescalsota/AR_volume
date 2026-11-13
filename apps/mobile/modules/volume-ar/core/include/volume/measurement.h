#pragma once

#include <vector>

#include "volume/geom.h"
#include "volume/plane.h"
#include "volume/solid.h"
#include "volume/triangulate.h"

namespace volume {

enum class HeightMode {
    Manual = 0,   // user supplies a height
    Taps,         // base and top tapped on the same vertical line
    RoofPlane     // roof plane fitted from tapped roof corners
};

struct HeightInput {
    HeightMode mode{HeightMode::Manual};
    double manualHeight{0.0};
    Vec3 base;
    Vec3 top;
    Plane roofPlane;
    Vec3 up{0.0, 1.0, 0.0};
};

struct MeasurementResult {
    double area{0.0};
    double perimeter{0.0};
    double height{0.0};
    double volume{0.0};
    double relativeUncertainty{0.0};  // 0..1, heuristic
    bool planarFootprint{false};
    double planarityRms{0.0};
    SolidStatus status{SolidStatus::Ok};

    bool ok() const { return status == SolidStatus::Ok; }
};

// Ground plane is derived from the footprint itself.
MeasurementResult computeMeasurement(const std::vector<Vec3>& footprint,
                                     const HeightInput& heightInput);

// Heuristic relative uncertainty from the landmark triangulations that define
// the corners. Returns 0..1; documented as a quality indicator, not a
// calibrated statistical bound.
double relativeUncertaintyFromLandmarks(
    const std::vector<TriangulationResult>& landmarks, double footprintArea);

}  // namespace volume
