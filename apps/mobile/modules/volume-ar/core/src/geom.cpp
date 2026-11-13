#include "volume/geom.h"

namespace volume {

Ray unprojectToWorldRay(const CameraIntrinsics& intr, const Mat4& cameraToWorld,
                        double px, double py) {
    Ray ray;
    ray.origin = cameraToWorld.transformPoint(Vec3{0.0, 0.0, 0.0});

    if (intr.fx <= kEps || intr.fy <= kEps) {
        ray.direction = Vec3{0.0, 0.0, -1.0};
        return ray;
    }

    const double nx = (px - intr.cx) / intr.fx;
    const double ny = (intr.cy - py) / intr.fy;  // pixel +y is down
    const Vec3 dirCam{nx, ny, -1.0};             // camera looks down -z
    const Vec3 dirWorld = cameraToWorld.transformDirection(dirCam);
    ray.direction = dirWorld.normalized();
    return ray;
}

}  // namespace volume
