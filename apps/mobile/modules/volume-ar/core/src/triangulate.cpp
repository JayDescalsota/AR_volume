#include "volume/triangulate.h"

#include <algorithm>
#include <cmath>

namespace volume {
namespace {

// Solves A x = b for a 3x3 system with partial pivoting.
bool solve3(double A[3][3], double b[3], double x[3]) {
    double M[3][4];
    double maxAbs = 0.0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            M[r][c] = A[r][c];
            maxAbs = std::max(maxAbs, std::fabs(A[r][c]));
        }
        M[r][3] = b[r];
    }
    if (maxAbs < 1e-12) return false;

    for (int col = 0; col < 3; ++col) {
        int pivot = col;
        double best = std::fabs(M[col][col]);
        for (int r = col + 1; r < 3; ++r) {
            if (std::fabs(M[r][col]) > best) {
                best = std::fabs(M[r][col]);
                pivot = r;
            }
        }
        if (best < 1e-10 * maxAbs) return false;
        if (pivot != col) {
            for (int c = col; c < 4; ++c) std::swap(M[col][c], M[pivot][c]);
        }
        const double inv = 1.0 / M[col][col];
        for (int r = col + 1; r < 3; ++r) {
            const double f = M[r][col] * inv;
            for (int c = col; c < 4; ++c) M[r][c] -= f * M[col][c];
        }
    }

    for (int r = 2; r >= 0; --r) {
        double s = M[r][3];
        for (int c = r + 1; c < 3; ++c) s -= M[r][c] * x[c];
        x[r] = s / M[r][r];
        if (!std::isfinite(x[r])) return false;
    }
    return true;
}

}  // namespace

double pointToRayDistance(const Vec3& p, const Ray& r) {
    const Vec3 d = r.direction.normalized();
    const Vec3 v = p - r.origin;
    const Vec3 perp = v - d * v.dot(d);
    return perp.norm();
}

TriangulationResult triangulate(const std::vector<Ray>& rays) {
    TriangulationResult out;
    out.rayCount = static_cast<int>(rays.size());
    if (rays.size() < 2) return out;

    double A[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    double b[3] = {0, 0, 0};

    double maxParallaxCos = -1.0;
    for (size_t i = 0; i < rays.size(); ++i) {
        const Vec3 d = rays[i].direction.normalized();
        const Vec3 o = rays[i].origin;

        const double dxy = d.x * d.y;
        const double dxz = d.x * d.z;
        const double dyz = d.y * d.z;
        const double P[3][3] = {
            {1.0 - d.x * d.x, -dxy, -dxz},
            {-dxy, 1.0 - d.y * d.y, -dyz},
            {-dxz, -dyz, 1.0 - d.z * d.z}};

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) A[r][c] += P[r][c];
            b[r] += P[r][0] * o.x + P[r][1] * o.y + P[r][2] * o.z;
        }

        for (size_t j = i + 1; j < rays.size(); ++j) {
            const Vec3 dj = rays[j].direction.normalized();
            maxParallaxCos = std::max(maxParallaxCos, clampd(d.dot(dj), -1.0, 1.0));
        }
    }

    double x[3];
    if (!solve3(A, b, x)) return out;

    out.point = Vec3{x[0], x[1], x[2]};
    if (!out.point.finite()) return out;

    double sumSq = 0.0;
    for (const auto& r : rays) {
        const double dist = pointToRayDistance(out.point, r);
        sumSq += dist * dist;
    }
    out.rmsResidual = std::sqrt(sumSq / static_cast<double>(rays.size()));

    for (size_t i = 0; i < rays.size(); ++i) {
        for (size_t j = i + 1; j < rays.size(); ++j) {
            out.baseline = std::max(out.baseline,
                                    distance(rays[i].origin, rays[j].origin));
        }
    }

    out.maxParallaxRad =
        maxParallaxCos < -0.5 ? 0.0 : std::acos(clampd(maxParallaxCos, -1.0, 1.0));

    const double parallaxScore = clampd(out.maxParallaxRad / (10.0 * kPi / 180.0), 0.0, 1.0);
    const double expectedResidual = std::max(0.005 * out.baseline, 0.002);
    const double ratio = out.rmsResidual / expectedResidual;
    const double residualScore = 1.0 / (1.0 + ratio * ratio);
    const double countScore = clampd(static_cast<double>(out.rayCount - 1) / 3.0, 0.0, 1.0);
    out.confidence = clampd(0.5 * parallaxScore + 0.3 * residualScore + 0.2 * countScore,
                            0.0, 1.0);

    out.valid = out.baseline > 1e-4 && out.confidence > 0.0;
    return out;
}

}  // namespace volume
