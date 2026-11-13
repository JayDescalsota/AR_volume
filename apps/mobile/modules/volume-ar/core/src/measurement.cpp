#include "volume/measurement.h"

#include <algorithm>
#include <cmath>

#include "volume/polygon.h"

namespace volume {

MeasurementResult computeMeasurement(const std::vector<Vec3>& rawFootprint,
                                     const HeightInput& heightInput) {
    MeasurementResult out;
    const std::vector<Vec3> footprint = cleanLoop(rawFootprint, 1e-4);
    if (footprint.size() < 3) {
        out.status = SolidStatus::DegeneratePolygon;
        return out;
    }

    double planarity = 0.0;
    out.planarFootprint = isPlanar(footprint, 0.10, &planarity);
    out.planarityRms = planarity;
    out.area = polygonArea(footprint);

    double perimeter = 0.0;
    for (double e : edgeLengths(footprint, true)) perimeter += e;
    out.perimeter = perimeter;

    VolumeEstimate estimate;
    switch (heightInput.mode) {
        case HeightMode::Manual:
            estimate = extrusionVolume(footprint, heightInput.manualHeight);
            break;
        case HeightMode::Taps: {
            const double h = heightFromTaps(heightInput.base, heightInput.top,
                                            heightInput.up);
            estimate = extrusionVolume(footprint, h);
            break;
        }
        case HeightMode::RoofPlane: {
            const Plane ground = planeFromLoop(footprint);
            estimate = planeBoundedVolume(footprint, ground, heightInput.roofPlane,
                                          heightInput.up);
            break;
        }
    }

    out.area = estimate.footprintArea > 0.0 ? estimate.footprintArea : out.area;
    out.height = estimate.height;
    out.volume = estimate.volume;
    out.status = estimate.status;
    return out;
}

double relativeUncertaintyFromLandmarks(
    const std::vector<TriangulationResult>& landmarks, double footprintArea) {
    if (landmarks.empty()) return 0.0;

    double maxSigma = 0.0;
    for (const auto& lm : landmarks) {
        if (!lm.valid) return 1.0;
        const double parallax =
            std::max(lm.maxParallaxRad, static_cast<double>(2.0 * kPi / 180.0));
        const double sigma = lm.rmsResidual /
                             std::sqrt(static_cast<double>(std::max(1, lm.rayCount))) /
                             std::sin(parallax);
        if (!std::isfinite(sigma)) return 1.0;
        maxSigma = std::max(maxSigma, sigma);
    }

    const double characteristic = std::sqrt(std::max(footprintArea, 1e-4));
    const double relative = 1.5 * maxSigma / characteristic + 0.01;
    return clampd(relative, 0.0, 1.0);
}

}  // namespace volume
