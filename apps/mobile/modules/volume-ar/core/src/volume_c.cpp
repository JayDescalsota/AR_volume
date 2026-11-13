#include "volume/volume_c.h"

#include <vector>

#include "volume/measurement.h"
#include "volume/polygon.h"
#include "volume/solid.h"
#include "volume/triangulate.h"

using namespace volume;

namespace {

std::vector<Vec3> readPoints(const double* data, int count) {
    std::vector<Vec3> out;
    if (!data || count <= 0) return out;
    out.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        out.emplace_back(data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2]);
    }
    return out;
}

int writeStats(const VolumeEstimate& estimate, double* out) {
    if (!out) return 0;
    out[0] = estimate.volume;
    out[1] = estimate.footprintArea;
    out[2] = estimate.height;
    out[3] = static_cast<double>(estimate.status);
    return estimate.ok() ? 1 : 0;
}

Plane readPlane(const double* p) {
    Plane plane;
    if (!p) return plane;
    plane.normal = Vec3{p[0], p[1], p[2]}.normalized();
    plane.d = p[3];
    return plane;
}

}  // namespace

extern "C" {

int vm_unproject_ray(const double* intrinsics4, const double* camToWorld16,
                     double px, double py, double* outOrigin3, double* outDir3) {
    if (!intrinsics4 || !camToWorld16 || !outOrigin3 || !outDir3) return 0;
    CameraIntrinsics intr;
    intr.fx = intrinsics4[0];
    intr.fy = intrinsics4[1];
    intr.cx = intrinsics4[2];
    intr.cy = intrinsics4[3];

    Mat4 m;
    for (int i = 0; i < 16; ++i) m.m[i] = camToWorld16[i];

    const Ray ray = unprojectToWorldRay(intr, m, px, py);
    if (!ray.direction.finite() || ray.direction.normSq() < 0.5) return 0;

    outOrigin3[0] = ray.origin.x;
    outOrigin3[1] = ray.origin.y;
    outOrigin3[2] = ray.origin.z;
    outDir3[0] = ray.direction.x;
    outDir3[1] = ray.direction.y;
    outDir3[2] = ray.direction.z;
    return 1;
}

int vm_triangulate(const double* rays6, int rayCount, double* outPoint3,
                   double* outStats4) {
    if (!rays6 || rayCount <= 0) return 0;
    std::vector<Ray> rays;
    rays.reserve(static_cast<size_t>(rayCount));
    for (int i = 0; i < rayCount; ++i) {
        Ray r;
        r.origin = Vec3{rays6[i * 6 + 0], rays6[i * 6 + 1], rays6[i * 6 + 2]};
        r.direction = Vec3{rays6[i * 6 + 3], rays6[i * 6 + 4], rays6[i * 6 + 5]};
        rays.push_back(r);
    }

    const TriangulationResult result = triangulate(rays);
    if (outPoint3) {
        outPoint3[0] = result.point.x;
        outPoint3[1] = result.point.y;
        outPoint3[2] = result.point.z;
    }
    if (outStats4) {
        outStats4[0] = result.rmsResidual;
        outStats4[1] = result.baseline;
        outStats4[2] = result.maxParallaxRad;
        outStats4[3] = result.confidence;
    }
    return result.valid ? 1 : 0;
}

double vm_polygon_area(const double* points3, int count) {
    return polygonArea(readPoints(points3, count));
}

int vm_volume_extrusion(const double* footprint3, int count, double height,
                        double* outStats4) {
    return writeStats(extrusionVolume(readPoints(footprint3, count), height),
                      outStats4);
}

int vm_volume_plane_bounded(const double* footprint3, int count,
                            const double* groundPlane4,
                            const double* roofPlane4, const double* up3,
                            double* outStats4) {
    const Vec3 up = up3 ? Vec3{up3[0], up3[1], up3[2]} : Vec3{0.0, 1.0, 0.0};
    return writeStats(planeBoundedVolume(readPoints(footprint3, count),
                                         readPlane(groundPlane4),
                                         readPlane(roofPlane4), up),
                      outStats4);
}

int vm_volume_polyhedron(const double* bottom3, const double* top3, int count,
                         double* outStats4) {
    return writeStats(polyhedronVolume(readPoints(bottom3, count),
                                       readPoints(top3, count)),
                      outStats4);
}

double vm_height_from_taps(const double* base3, const double* top3,
                           const double* up3) {
    if (!base3 || !top3) return 0.0;
    const Vec3 base{base3[0], base3[1], base3[2]};
    const Vec3 top{top3[0], top3[1], top3[2]};
    const Vec3 up = up3 ? Vec3{up3[0], up3[1], up3[2]} : Vec3{0.0, 1.0, 0.0};
    return heightFromTaps(base, top, up);
}

void vm_shutdown(void) {}

}  // extern "C"
