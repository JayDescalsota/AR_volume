#include <vector>

#include "test_common.h"
#include "volume/measurement.h"
#include "volume/polygon.h"
#include "volume/solid.h"

using namespace volume;

namespace {

std::vector<Vec3> square(double side, double z) {
    return {{0, 0, z}, {side, 0, z}, {side, side, z}, {0, side, z}};
}

}  // namespace

int runVolumeTests() {
    // --- extrusion --------------------------------------------------------
    VolumeEstimate box = extrusionVolume(square(2.0, 0.0), 3.0);
    CHECK(box.ok());
    CHECK_NEAR(box.footprintArea, 4.0, 1e-12);
    CHECK_NEAR(box.volume, 12.0, 1e-12);

    VolumeEstimate zeroHeight = extrusionVolume(square(2.0, 0.0), 0.0);
    CHECK(!zeroHeight.ok());

    VolumeEstimate degenerate = extrusionVolume({{0, 0, 0}, {1, 0, 0}}, 3.0);
    CHECK(degenerate.status == SolidStatus::DegeneratePolygon);

    // --- plane-bounded (flat roof) ---------------------------------------
    const Plane ground = planeFromNormalPoint(Vec3{0, 0, 1}, Vec3{0, 0, 0});
    const Plane flatRoof = planeFromNormalPoint(Vec3{0, 0, 1}, Vec3{0, 0, 3});
    VolumeEstimate flat = planeBoundedVolume(square(2.0, 0.0), ground, flatRoof,
                                             Vec3{0, 0, 1});
    CHECK(flat.ok());
    CHECK_NEAR(flat.height, 3.0, 1e-9);
    CHECK_NEAR(flat.volume, 12.0, 1e-9);

    // --- plane-bounded (sloped roof) -------------------------------------
    // Roof plane z = 3 + x over [0,2]x[0,2]: mean height is 3 + mean(x) = 4.
    const Plane slopedRoof =
        planeFromNormalPoint(Vec3{-1, 0, 1}, Vec3{0, 0, 3});
    VolumeEstimate sloped = planeBoundedVolume(square(2.0, 0.0), ground,
                                               slopedRoof, Vec3{0, 0, 1});
    CHECK(sloped.ok());
    CHECK_NEAR(sloped.height, 4.0, 1e-6);
    CHECK_NEAR(sloped.volume, 16.0, 1e-6);

    // A roof below the ground is invalid.
    const Plane below = planeFromNormalPoint(Vec3{0, 0, 1}, Vec3{0, 0, -2});
    CHECK(!planeBoundedVolume(square(2.0, 0.0), ground, below, Vec3{0, 0, 1}).ok());

    // --- general polyhedron (flat top) -----------------------------------
    VolumeEstimate prism = polyhedronVolume(square(2.0, 0.0), square(2.0, 3.0));
    CHECK(prism.ok());
    CHECK_NEAR(prism.volume, 12.0, 1e-9);

    // --- general polyhedron (sloped top) ---------------------------------
    std::vector<Vec3> top = square(2.0, 0.0);
    for (auto& p : top) {
        p.z = 3.0 + p.x;  // sloped roof, mean height 4 over the square
    }
    VolumeEstimate slopedPrism = polyhedronVolume(square(2.0, 0.0), top);
    CHECK(slopedPrism.ok());
    CHECK_NEAR(slopedPrism.volume, 16.0, 1e-9);

    // --- loop winding must not change the magnitude ----------------------
    // Both loops reversed consistently: correspondence is preserved.
    std::vector<Vec3> reversedBottom = square(2.0, 0.0);
    std::swap(reversedBottom[1], reversedBottom[3]);
    std::vector<Vec3> reversedTop = square(2.0, 3.0);
    std::swap(reversedTop[1], reversedTop[3]);
    VolumeEstimate reversedPrism =
        polyhedronVolume(reversedBottom, reversedTop);
    CHECK(reversedPrism.ok());
    CHECK_NEAR(reversedPrism.volume, 12.0, 1e-9);

    // --- mismatched loops -------------------------------------------------
    std::vector<Vec3> triangle{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    CHECK(polyhedronVolume(triangle, square(2.0, 3.0)).status ==
          SolidStatus::MismatchedLoops);

    // --- height from taps -------------------------------------------------
    CHECK_NEAR(heightFromTaps(Vec3{0, 0, 0}, Vec3{0, 0, 4}, Vec3{0, 0, 1}),
               4.0, 1e-12);
    // Horizontal offset does not contribute to vertical extent.
    CHECK_NEAR(heightFromTaps(Vec3{0, 0, 0}, Vec3{5, 0, 4}, Vec3{0, 0, 1}),
               4.0, 1e-12);

    // --- full measurement dispatch ---------------------------------------
    HeightInput manual;
    manual.mode = HeightMode::Manual;
    manual.manualHeight = 5.0;
    MeasurementResult m = computeMeasurement(square(2.0, 0.0), manual);
    CHECK(m.ok());
    CHECK_NEAR(m.area, 4.0, 1e-12);
    CHECK_NEAR(m.perimeter, 8.0, 1e-12);
    CHECK_NEAR(m.volume, 20.0, 1e-12);
    CHECK(m.planarFootprint);

    HeightInput taps;
    taps.mode = HeightMode::Taps;
    taps.base = Vec3{0, 0, 0};
    taps.top = Vec3{0, 0, 2.5};
    taps.up = Vec3{0, 0, 1};
    MeasurementResult mt = computeMeasurement(square(2.0, 0.0), taps);
    CHECK(mt.ok());
    CHECK_NEAR(mt.volume, 10.0, 1e-9);

    HeightInput roof;
    roof.mode = HeightMode::RoofPlane;
    roof.roofPlane = planeFromNormalPoint(Vec3{0, 0, 1}, Vec3{0, 0, 1.5});
    roof.up = Vec3{0, 0, 1};
    MeasurementResult mr = computeMeasurement(square(2.0, 0.0), roof);
    CHECK(mr.ok());
    CHECK_NEAR(mr.volume, 6.0, 1e-9);

    // --- uncertainty heuristic -------------------------------------------
    TriangulationResult good;
    good.valid = true;
    good.rayCount = 3;
    good.rmsResidual = 0.005;
    good.maxParallaxRad = 20.0 * kPi / 180.0;
    CHECK(relativeUncertaintyFromLandmarks({good}, 100.0) < 0.1);

    TriangulationResult bad;
    bad.valid = false;
    CHECK_NEAR(relativeUncertaintyFromLandmarks({bad}, 100.0), 1.0, 1e-12);

    return 0;
}
