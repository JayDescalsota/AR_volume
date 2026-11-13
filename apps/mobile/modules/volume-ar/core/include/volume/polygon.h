#pragma once

#include <vector>

#include "volume/geom.h"

namespace volume {

// Newell vector; its length is twice the projected polygon area.
Vec3 polygonAreaVector(const std::vector<Vec3>& loop);

double polygonArea(const std::vector<Vec3>& loop);

// Area-weighted centroid. Correct for planar simple polygons, including
// concave ones; for non-planar input this is the projected centroid.
Vec3 polygonCentroid(const std::vector<Vec3>& loop);

// True when every vertex lies within `tolerance` metres of the best-fit plane.
bool isPlanar(const std::vector<Vec3>& loop, double tolerance,
              double* outRms = nullptr);

// Edge lengths; `closed` appends the closing edge back to the first vertex.
std::vector<double> edgeLengths(const std::vector<Vec3>& loop, bool closed);

// Removes consecutive duplicate vertices closer than `minSpacing` metres and
// optionally the closing duplicate.
std::vector<Vec3> cleanLoop(const std::vector<Vec3>& loop, double minSpacing);

}  // namespace volume
