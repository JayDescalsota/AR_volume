#include "volume/polygon.h"

#include <algorithm>
#include <cmath>

#include "volume/plane.h"

namespace volume {

Vec3 polygonAreaVector(const std::vector<Vec3>& loop) {
    Vec3 sum;
    if (loop.size() < 3) return sum;
    for (size_t i = 0; i < loop.size(); ++i) {
        const Vec3& a = loop[i];
        const Vec3& b = loop[(i + 1) % loop.size()];
        sum += a.cross(b);
    }
    return sum * 0.5;
}

double polygonArea(const std::vector<Vec3>& loop) {
    return polygonAreaVector(loop).norm();
}

Vec3 polygonCentroid(const std::vector<Vec3>& loop) {
    if (loop.empty()) return Vec3{};
    if (loop.size() < 3) {
        Vec3 c;
        for (const auto& p : loop) c += p;
        return c / static_cast<double>(loop.size());
    }

    const Vec3 areaVec = polygonAreaVector(loop);
    if (areaVec.norm() < 1e-12) {
        Vec3 c;
        for (const auto& p : loop) c += p;
        return c / static_cast<double>(loop.size());
    }
    const Vec3 n = areaVec.normalized();

    const Vec3& r = loop[0];
    double area2 = 0.0;
    Vec3 weighted;
    for (size_t i = 1; i + 1 < loop.size(); ++i) {
        const Vec3& a = loop[i];
        const Vec3& b = loop[i + 1];
        const double signedArea =
            0.5 * ((a - r).cross(b - r)).dot(n);
        const Vec3 triCentroid = (r + a + b) / 3.0;
        area2 += signedArea;
        weighted += triCentroid * signedArea;
    }

    if (std::fabs(area2) < 1e-12) {
        Vec3 c;
        for (const auto& p : loop) c += p;
        return c / static_cast<double>(loop.size());
    }
    return weighted / area2;
}

bool isPlanar(const std::vector<Vec3>& loop, double tolerance, double* outRms) {
    if (loop.size() < 3) {
        if (outRms) *outRms = 0.0;
        return true;
    }
    const Plane plane = bestFitPlane(loop);
    const double rms = planarityRms(loop, plane);
    if (outRms) *outRms = rms;
    return rms <= tolerance;
}

std::vector<double> edgeLengths(const std::vector<Vec3>& loop, bool closed) {
    std::vector<double> lengths;
    if (loop.size() < 2) return lengths;
    for (size_t i = 0; i + 1 < loop.size(); ++i) {
        lengths.push_back(distance(loop[i], loop[i + 1]));
    }
    if (closed && loop.size() > 2) {
        lengths.push_back(distance(loop.back(), loop.front()));
    }
    return lengths;
}

std::vector<Vec3> cleanLoop(const std::vector<Vec3>& loop, double minSpacing) {
    std::vector<Vec3> out;
    for (const auto& p : loop) {
        if (!p.finite()) continue;
        if (!out.empty() && distance(out.back(), p) < minSpacing) continue;
        out.push_back(p);
    }
    while (out.size() > 1 && distance(out.front(), out.back()) < minSpacing) {
        out.pop_back();
    }
    return out;
}

}  // namespace volume
