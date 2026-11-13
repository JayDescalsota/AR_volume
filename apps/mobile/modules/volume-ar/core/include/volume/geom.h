#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace volume {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEps = 1e-9;

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    Vec3() = default;
    constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }

    double dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }

    double normSq() const { return x * x + y * y + z * z; }
    double norm() const { return std::sqrt(normSq()); }

    Vec3 normalized() const {
        const double n = norm();
        return n > kEps ? (*this) / n : Vec3{0.0, 0.0, 0.0};
    }

    bool finite() const {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
};

inline Vec3 operator*(double s, const Vec3& v) { return v * s; }
inline double distance(const Vec3& a, const Vec3& b) { return (a - b).norm(); }
inline double clampd(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Row-major 4x4 matrix. Layout: m[row * 4 + col].
struct Mat4 {
    double m[16]{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};

    Vec3 transformPoint(const Vec3& p) const {
        return {m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3],
                m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7],
                m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11]};
    }

    Vec3 transformDirection(const Vec3& d) const {
        return {m[0] * d.x + m[1] * d.y + m[2] * d.z,
                m[4] * d.x + m[5] * d.y + m[6] * d.z,
                m[8] * d.x + m[9] * d.y + m[10] * d.z};
    }
};

struct Ray {
    Vec3 origin;
    Vec3 direction;  // expected unit length
};

struct CameraIntrinsics {
    double fx{0.0};
    double fy{0.0};
    double cx{0.0};
    double cy{0.0};
    int width{0};
    int height{0};
};

// Pixel origin is top-left, +x right, +y down.
// Camera space is +x right, +y up, -z forward (OpenGL / ARKit / ARCore convention).
// `cameraToWorld` maps camera space into world space.
Ray unprojectToWorldRay(const CameraIntrinsics& intr, const Mat4& cameraToWorld,
                        double px, double py);

struct Plane {
    Vec3 normal{0.0, 1.0, 0.0};  // unit normal
    double d{0.0};               // n . x + d = 0

    double signedDistance(const Vec3& p) const { return normal.dot(p) + d; }
    Vec3 project(const Vec3& p) const { return p - normal * signedDistance(p); }
};

}  // namespace volume
