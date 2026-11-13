#include <vector>

#include "test_common.h"
#include "volume/geom.h"
#include "volume/plane.h"
#include "volume/polygon.h"
#include "volume/triangulate.h"

using namespace volume;

namespace {

Ray rayToward(const Vec3& origin, const Vec3& target) {
    Ray r;
    r.origin = origin;
    r.direction = (target - origin).normalized();
    return r;
}

}  // namespace

int runMathTests() {
    // --- unprojection -----------------------------------------------------
    CameraIntrinsics intr;
    intr.fx = 500.0;
    intr.fy = 500.0;
    intr.cx = 960.0;
    intr.cy = 540.0;
    intr.width = 1920;
    intr.height = 1080;

    Mat4 identity;
    Ray center = unprojectToWorldRay(intr, identity, 960.0, 540.0);
    CHECK_NEAR(center.direction.x, 0.0, 1e-12);
    CHECK_NEAR(center.direction.y, 0.0, 1e-12);
    CHECK_NEAR(center.direction.z, -1.0, 1e-12);

    Ray right = unprojectToWorldRay(intr, identity, 960.0 + 500.0, 540.0);
    CHECK_NEAR(right.direction.x, std::sqrt(0.5), 1e-9);
    CHECK_NEAR(right.direction.z, -std::sqrt(0.5), 1e-9);

    // Identity camera -> rays originate at the camera origin.
    CHECK_NEAR(center.origin.norm(), 0.0, 1e-12);

    // --- triangulation ----------------------------------------------------
    const Vec3 target{2.0, -1.0, 5.0};
    std::vector<Ray> rays{
        rayToward(Vec3{0.0, 0.0, 0.0}, target),
        rayToward(Vec3{1.0, 0.2, 0.0}, target),
        rayToward(Vec3{0.5, -0.4, 0.1}, target),
    };
    TriangulationResult tri = triangulate(rays);
    CHECK(tri.valid);
    CHECK_NEAR(tri.point.x, target.x, 1e-6);
    CHECK_NEAR(tri.point.y, target.y, 1e-6);
    CHECK_NEAR(tri.point.z, target.z, 1e-6);
    CHECK(tri.confidence > 0.5);

    // A single ray has unobservable depth and must be invalid.
    TriangulationResult single = triangulate({rays[0]});
    CHECK(!single.valid);

    // Coincident origins leave depth unobservable.
    std::vector<Ray> sameOrigin{
        rayToward(Vec3{0.0, 0.0, 0.0}, target),
        rayToward(Vec3{0.0, 0.0, 0.0}, target + Vec3{0.1, 0.0, 0.0}),
    };
    CHECK(!triangulate(sameOrigin).valid);

    // --- polygon area -----------------------------------------------------
    std::vector<Vec3> unitSquare{
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    CHECK_NEAR(polygonArea(unitSquare), 1.0, 1e-12);

    std::vector<Vec3> tilted{
        {0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}};
    // Rotating a planar polygon in 3D preserves its true area.
    std::vector<Vec3> rotated;
    for (const auto& p : tilted) {
        const double c = std::cos(kPi / 3.0);
        const double s = std::sin(kPi / 3.0);
        rotated.push_back({p.x, c * p.y - s * p.z, s * p.y + c * p.z});
    }
    CHECK_NEAR(polygonArea(rotated), 4.0, 1e-9);

    // Centroid of the symmetric square is the origin-ish centre.
    Vec3 c = polygonCentroid(unitSquare);
    CHECK_NEAR(c.x, 0.5, 1e-12);
    CHECK_NEAR(c.y, 0.5, 1e-12);

    // Concave L-shape: 3x1 plus 1x2 = 5 square units.
    std::vector<Vec3> lShape{
        {0, 0, 0}, {3, 0, 0}, {3, 1, 0}, {1, 1, 0}, {1, 3, 0}, {0, 3, 0}};
    CHECK_NEAR(polygonArea(lShape), 5.0, 1e-12);

    // --- planarity --------------------------------------------------------
    std::vector<Vec3> flat{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    double rms = 0.0;
    CHECK(isPlanar(flat, 1e-9, &rms));
    CHECK_NEAR(rms, 0.0, 1e-12);

    std::vector<Vec3> warped{{0, 0, 0}, {1, 0, 0}, {1, 1, 0.2}, {0, 1, 0}};
    double warpedRms = 0.0;
    CHECK(!isPlanar(warped, 0.01, &warpedRms));
    CHECK(warpedRms > 0.01);

    // --- plane fitting ----------------------------------------------------
    const Plane ground = planeFromLoop(flat);
    CHECK(std::fabs(std::fabs(ground.normal.z) - 1.0) < 1e-9);
    CHECK_NEAR(ground.signedDistance(Vec3{0.5, 0.5, 0.0}), 0.0, 1e-9);

    return 0;
}
