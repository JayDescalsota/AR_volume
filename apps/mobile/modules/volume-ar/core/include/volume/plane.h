#pragma once

#include <vector>

#include "volume/geom.h"

namespace volume {

Plane planeFromNormalPoint(const Vec3& normal, const Vec3& point);

// Least-squares plane through unordered points (smallest-eigenvector of the
// covariance matrix). Returns a default horizontal plane when degenerate.
Plane bestFitPlane(const std::vector<Vec3>& points);

// Plane through an ordered loop using the Newell normal.
Plane planeFromLoop(const std::vector<Vec3>& loop);

// RMS distance of points to the plane.
double planarityRms(const std::vector<Vec3>& points, const Plane& plane);

}  // namespace volume
