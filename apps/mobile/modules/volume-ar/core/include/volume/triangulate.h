#pragma once

#include <vector>

#include "volume/geom.h"

namespace volume {

struct TriangulationResult {
    Vec3 point;
    double rmsResidual{0.0};   // metres, RMS distance from point to each ray
    double baseline{0.0};      // max pairwise distance between ray origins
    double maxParallaxRad{0.0};
    double confidence{0.0};    // 0..1 heuristic
    int rayCount{0};
    bool valid{false};
};

// Least-squares intersection of one or more rays.
// With a single ray the result is invalid (depth is unobservable).
// Confidence combines parallax, residuals and ray count; it is a heuristic
// quality score, not a statistical confidence interval.
TriangulationResult triangulate(const std::vector<Ray>& rays);

double pointToRayDistance(const Vec3& p, const Ray& r);

}  // namespace volume
