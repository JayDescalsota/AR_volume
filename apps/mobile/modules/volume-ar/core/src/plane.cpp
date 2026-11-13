#include "volume/plane.h"

#include <algorithm>
#include <cmath>

namespace volume {
namespace {

// Cyclic Jacobi eigen decomposition for a real symmetric 3x3 matrix.
// A is overwritten; eigenvectors are written column-wise into V; eigenvalues
// into d.
void jacobiEigen(double A[3][3], double V[3][3], double d[3]) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) V[r][c] = (r == c) ? 1.0 : 0.0;
    }

    for (int iter = 0; iter < 32; ++iter) {
        const double off = A[0][1] * A[0][1] + A[0][2] * A[0][2] + A[1][2] * A[1][2];
        if (off < 1e-24) break;

        for (int p = 0; p < 2; ++p) {
            for (int q = p + 1; q < 3; ++q) {
                if (std::fabs(A[p][q]) < 1e-18) continue;

                const double theta = (A[q][q] - A[p][p]) / (2.0 * A[p][q]);
                const double t = (theta >= 0.0 ? 1.0 : -1.0) /
                                 (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0);
                const double s = t * c;

                for (int k = 0; k < 3; ++k) {
                    const double akp = A[k][p];
                    const double akq = A[k][q];
                    A[k][p] = c * akp - s * akq;
                    A[k][q] = s * akp + c * akq;
                }
                for (int k = 0; k < 3; ++k) {
                    const double apk = A[p][k];
                    const double aqk = A[q][k];
                    A[p][k] = c * apk - s * aqk;
                    A[q][k] = s * apk + c * aqk;
                }
                A[p][q] = 0.0;
                A[q][p] = 0.0;

                for (int k = 0; k < 3; ++k) {
                    const double vkp = V[k][p];
                    const double vkq = V[k][q];
                    V[k][p] = c * vkp - s * vkq;
                    V[k][q] = s * vkp + c * vkq;
                }
            }
        }
    }

    d[0] = A[0][0];
    d[1] = A[1][1];
    d[2] = A[2][2];
}

}  // namespace

Plane planeFromNormalPoint(const Vec3& normal, const Vec3& point) {
    Plane p;
    p.normal = normal.normalized();
    if (p.normal.normSq() < 0.5) p.normal = Vec3{0.0, 1.0, 0.0};
    p.d = -p.normal.dot(point);
    return p;
}

Plane bestFitPlane(const std::vector<Vec3>& points) {
    Plane plane;
    if (points.size() < 3) return plane;

    Vec3 centroid;
    for (const auto& p : points) centroid += p;
    centroid = centroid / static_cast<double>(points.size());

    double C[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    for (const auto& p : points) {
        const Vec3 q = p - centroid;
        C[0][0] += q.x * q.x;
        C[0][1] += q.x * q.y;
        C[0][2] += q.x * q.z;
        C[1][1] += q.y * q.y;
        C[1][2] += q.y * q.z;
        C[2][2] += q.z * q.z;
    }
    C[1][0] = C[0][1];
    C[2][0] = C[0][2];
    C[2][1] = C[1][2];

    double V[3][3];
    double d[3];
    jacobiEigen(C, V, d);

    int minIdx = 0;
    for (int i = 1; i < 3; ++i) {
        if (d[i] < d[minIdx]) minIdx = i;
    }

    Vec3 normal{V[0][minIdx], V[1][minIdx], V[2][minIdx]};
    normal = normal.normalized();
    if (normal.normSq() < 0.5) {
        normal = Vec3{0.0, 1.0, 0.0};
    }
    plane.normal = normal;
    plane.d = -normal.dot(centroid);
    return plane;
}

Plane planeFromLoop(const std::vector<Vec3>& loop) {
    if (loop.size() < 3) return Plane{};

    Vec3 newell;
    Vec3 centroid;
    for (size_t i = 0; i < loop.size(); ++i) {
        const Vec3& a = loop[i];
        const Vec3& b = loop[(i + 1) % loop.size()];
        newell += a.cross(b);
        centroid += a;
    }
    centroid = centroid / static_cast<double>(loop.size());

    if (newell.norm() < 1e-12) return bestFitPlane(loop);
    return planeFromNormalPoint(newell, centroid);
}

double planarityRms(const std::vector<Vec3>& points, const Plane& plane) {
    if (points.empty()) return 0.0;
    double sumSq = 0.0;
    for (const auto& p : points) {
        const double d = plane.signedDistance(p);
        sumSq += d * d;
    }
    return std::sqrt(sumSq / static_cast<double>(points.size()));
}

}  // namespace volume
