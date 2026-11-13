#include "volume/solid.h"

#include <cmath>
#include <vector>

#include "volume/polygon.h"

namespace volume {
namespace {

bool polygonUsable(const std::vector<Vec3>& loop, double* areaOut) {
    if (loop.size() < 3) return false;
    for (const auto& p : loop) {
        if (!p.finite()) return false;
    }
    const double area = polygonArea(loop);
    if (areaOut) *areaOut = area;
    return area > 1e-6;
}

struct MeshVolumeAccumulator {
    double sum{0.0};
    void addTriangle(const Vec3& a, const Vec3& b, const Vec3& c) {
        sum += a.dot(b.cross(c)) / 6.0;
    }
};

}  // namespace

double heightFromTaps(const Vec3& base, const Vec3& top, const Vec3& up) {
    const Vec3 u = up.normalized();
    if (u.normSq() < 0.5) return distance(base, top);
    return std::fabs((top - base).dot(u));
}

VolumeEstimate extrusionVolume(const std::vector<Vec3>& footprint,
                               double height) {
    VolumeEstimate out;
    double area = 0.0;
    if (!polygonUsable(footprint, &area)) {
        out.status = SolidStatus::DegeneratePolygon;
        return out;
    }
    if (!std::isfinite(height) || height <= 1e-6) {
        out.status = SolidStatus::DegenerateHeight;
        return out;
    }
    out.footprintArea = area;
    out.height = height;
    out.volume = area * height;
    out.status = SolidStatus::Ok;
    return out;
}

VolumeEstimate planeBoundedVolume(const std::vector<Vec3>& footprint,
                                  const Plane& groundIn, const Plane& roofIn,
                                  const Vec3& upIn) {
    VolumeEstimate out;
    double area = 0.0;
    if (!polygonUsable(footprint, &area)) {
        out.status = SolidStatus::DegeneratePolygon;
        return out;
    }

    const Vec3 up = upIn.normalized();
    if (up.normSq() < 0.5) {
        out.status = SolidStatus::DegeneratePlane;
        return out;
    }

    Plane ground = groundIn;
    Plane roof = roofIn;
    ground.normal = ground.normal.normalized();
    roof.normal = roof.normal.normalized();
    if (ground.normal.normSq() < 0.5 || roof.normal.normSq() < 0.5) {
        out.status = SolidStatus::DegeneratePlane;
        return out;
    }

    // Place the area centroid on the ground plane, then walk along `up` until
    // the roof plane is reached. Height varies linearly over a planar roof, so
    // the mean height equals the height at the area centroid.
    const Vec3 centroid = polygonCentroid(footprint);
    const Vec3 groundPoint = centroid - ground.normal * ground.signedDistance(centroid);

    const double denom = roof.normal.dot(up);
    if (std::fabs(denom) < 1e-6) {
        out.status = SolidStatus::DegeneratePlane;
        return out;
    }

    const double t = -(roof.normal.dot(groundPoint) + roof.d) / denom;
    if (!std::isfinite(t) || t <= 1e-6) {
        out.status = SolidStatus::DegenerateHeight;
        return out;
    }

    out.footprintArea = area;
    out.height = t;
    out.volume = area * t;
    out.status = SolidStatus::Ok;
    return out;
}

VolumeEstimate polyhedronVolume(const std::vector<Vec3>& bottom,
                                const std::vector<Vec3>& top) {
    VolumeEstimate out;
    if (bottom.size() != top.size()) {
        out.status = SolidStatus::MismatchedLoops;
        return out;
    }
    if (bottom.size() < 3) {
        out.status = SolidStatus::DegeneratePolygon;
        return out;
    }
    for (size_t i = 0; i < bottom.size(); ++i) {
        if (!bottom[i].finite() || !top[i].finite()) {
            out.status = SolidStatus::NonFinite;
            return out;
        }
    }

    const double area = polygonArea(bottom);
    if (area <= 1e-6) {
        out.status = SolidStatus::DegeneratePolygon;
        return out;
    }

    const int n = static_cast<int>(bottom.size());
    MeshVolumeAccumulator mesh;

    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        mesh.addTriangle(bottom[i], bottom[j], top[j]);
        mesh.addTriangle(bottom[i], top[j], top[i]);
    }
    for (int i = 1; i + 1 < n; ++i) {
        mesh.addTriangle(top[0], top[i], top[i + 1]);
        mesh.addTriangle(bottom[0], bottom[i + 1], bottom[i]);
    }

    const double signedVolume = mesh.sum;
    if (!std::isfinite(signedVolume)) {
        out.status = SolidStatus::NonFinite;
        return out;
    }

    double heightSum = 0.0;
    for (int i = 0; i < n; ++i) heightSum += distance(bottom[i], top[i]);

    out.footprintArea = area;
    out.height = heightSum / static_cast<double>(n);
    out.volume = std::fabs(signedVolume);
    out.status = out.volume > 1e-9 ? SolidStatus::Ok : SolidStatus::DegenerateHeight;
    return out;
}

}  // namespace volume
