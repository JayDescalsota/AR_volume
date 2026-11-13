#pragma once

#include <vector>

#include "volume/geom.h"
#include "volume/plane.h"

namespace volume {

enum class SolidStatus {
    Ok = 0,
    DegeneratePolygon,
    DegenerateHeight,
    DegeneratePlane,
    MismatchedLoops,
    NonFinite
};

struct VolumeEstimate {
    double volume{0.0};        // cubic metres
    double footprintArea{0.0}; // square metres
    double height{0.0};        // metres, representative height
    SolidStatus status{SolidStatus::Ok};

    bool ok() const { return status == SolidStatus::Ok; }
};

// Footprint extruded by a constant height.
VolumeEstimate extrusionVolume(const std::vector<Vec3>& footprint,
                               double height);

// Volume between a (planar) footprint on `ground` and a planar roof. Exact for
// a prismatoid because a plane's height is linear over the footprint, so the
// mean height equals the height at the area centroid.
VolumeEstimate planeBoundedVolume(const std::vector<Vec3>& footprint,
                                  const Plane& ground, const Plane& roof,
                                  const Vec3& up);

// General solid between two corresponding loops (same count, vertex i of the
// bottom corresponds to vertex i of the top). Handles sloped and irregular
// roofs. Uses a closed triangle mesh and the divergence theorem.
VolumeEstimate polyhedronVolume(const std::vector<Vec3>& bottom,
                                const std::vector<Vec3>& top);

// Vertical extent between two tapped points along `up`.
double heightFromTaps(const Vec3& base, const Vec3& top, const Vec3& up);

}  // namespace volume
