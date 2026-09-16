#pragma once

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

namespace samp_editor {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

struct Vec2 {
    float x{0.0f}, y{0.0f};
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};
    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float Dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 Cross(const Vec3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }
    float LengthSq() const { return x * x + y * y + z * z; }
    float Length() const { return std::sqrt(LengthSq()); }
    Vec3 Normalized() const {
        float l = Length();
        if (l > 1e-6f) return *this / l;
        return {0.0f, 0.0f, 0.0f};
    }
};

struct Vec4 {
    float x{0.0f}, y{0.0f}, z{0.0f}, w{1.0f};
    Vec4() = default;
    Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    Vec4(const Vec3& v, float w_ = 1.0f) : x(v.x), y(v.y), z(v.z), w(w_) {}
};

// Quaternion: q = w + xi + yj + zk
struct Quat {
    float x{0.0f}, y{0.0f}, z{0.0f}, w{1.0f};

    Quat() = default;
    Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    static Quat Identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }

    Quat operator*(const Quat& q) const {
        return {
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        };
    }

    Quat Normalized() const {
        float l = std::sqrt(x * x + y * y + z * z + w * w);
        if (l > 1e-6f) return {x / l, y / l, z / l, w / l};
        return Identity();
    }

    // SA-MP & GTA SA standard:
    // Euler angles in degrees (rX, rY, rZ).
    // Rotation order: Yaw (Z), Pitch (X), Roll (Y)
    static Quat FromEulerGTA(float rx_deg, float ry_deg, float rz_deg) {
        float hx = (rx_deg * DEG_TO_RAD) * 0.5f;
        float hy = (ry_deg * DEG_TO_RAD) * 0.5f;
        float hz = (rz_deg * DEG_TO_RAD) * 0.5f;

        float cx = std::cos(hx), sx = std::sin(hx);
        float cy = std::cos(hy), sy = std::sin(hy);
        float cz = std::cos(hz), sz = std::sin(hz);

        Quat qz(0.0f, 0.0f, sz, cz);
        Quat qx(sx, 0.0f, 0.0f, cx);
        Quat qy(0.0f, sy, 0.0f, cy);

        return (qz * qx * qy).Normalized();
    }

    // Extract Euler angles in degrees from Quaternion (GTA SA convention: Rz * Rx * Ry)
    Vec3 ToEulerGTA() const {
        float m21 = 2.0f * (y * z + w * x);
        float m20 = 2.0f * (x * z - w * y);
        float m22 = 1.0f - 2.0f * (x * x + y * y);
        float m01 = 2.0f * (x * y - w * z);
        float m11 = 1.0f - 2.0f * (x * x + z * z);

        float sinPitch = std::clamp(m21, -1.0f, 1.0f);
        float rx = std::asin(sinPitch);
        float ry = std::atan2(-m20, m22);
        float rz = std::atan2(-m01, m11);

        return {rx * RAD_TO_DEG, ry * RAD_TO_DEG, rz * RAD_TO_DEG};
    }
};

// Column-major 4x4 Matrix (OpenGL standard)
struct Mat4 {
    float m[16];

    Mat4() {
        Identity();
    }

    void Identity() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Mat4 Translation(const Vec3& t) {
        Mat4 res;
        res.m[12] = t.x;
        res.m[13] = t.y;
        res.m[14] = t.z;
        return res;
    }

    static Mat4 Scale(const Vec3& s) {
        Mat4 res;
        res.m[0] = s.x;
        res.m[5] = s.y;
        res.m[10] = s.z;
        return res;
    }

    static Mat4 FromQuat(const Quat& q) {
        Mat4 res;
        float x2 = q.x * 2.0f, y2 = q.y * 2.0f, z2 = q.z * 2.0f;
        float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2;
        float yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
        float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;

        res.m[0] = 1.0f - (yy + zz);
        res.m[1] = xy + wz;
        res.m[2] = xz - wy;
        res.m[3] = 0.0f;

        res.m[4] = xy - wz;
        res.m[5] = 1.0f - (xx + zz);
        res.m[6] = yz + wx;
        res.m[7] = 0.0f;

        res.m[8] = xz + wy;
        res.m[9] = yz - wx;
        res.m[10] = 1.0f - (xx + yy);
        res.m[11] = 0.0f;

        res.m[12] = 0.0f;
        res.m[13] = 0.0f;
        res.m[14] = 0.0f;
        res.m[15] = 1.0f;
        return res;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 res;
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                res.m[c * 4 + r] =
                    m[0 * 4 + r] * o.m[c * 4 + 0] +
                    m[1 * 4 + r] * o.m[c * 4 + 1] +
                    m[2 * 4 + r] * o.m[c * 4 + 2] +
                    m[3 * 4 + r] * o.m[c * 4 + 3];
            }
        }
        return res;
    }

    Vec3 TransformPoint(const Vec3& p) const {
        float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
        if (std::abs(w) < 1e-6f) w = 1.0f;
        return {
            (m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12]) / w,
            (m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13]) / w,
            (m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]) / w
        };
    }

    static Mat4 Perspective(float fovRad, float aspect, float nearZ, float farZ) {
        Mat4 res;
        for (int i = 0; i < 16; ++i) res.m[i] = 0.0f;
        float tanHalfFov = std::tan(fovRad * 0.5f);

        res.m[0] = 1.0f / (aspect * tanHalfFov);
        res.m[5] = 1.0f / tanHalfFov;
        res.m[10] = -(farZ + nearZ) / (farZ - nearZ);
        res.m[11] = -1.0f;
        res.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        return res;
    }

    // LookAt dengan Z-up (Standar GTA San Andreas)
    static Mat4 LookAtGTA(const Vec3& eye, const Vec3& target, const Vec3& up = {0.0f, 0.0f, 1.0f}) {
        Vec3 forward = (target - eye).Normalized();
        Vec3 right = forward.Cross(up).Normalized();
        Vec3 actualUp = right.Cross(forward);

        Mat4 res;
        res.m[0] = right.x;
        res.m[4] = right.y;
        res.m[8] = right.z;
        res.m[12] = -right.Dot(eye);

        res.m[1] = actualUp.x;
        res.m[5] = actualUp.y;
        res.m[9] = actualUp.z;
        res.m[13] = -actualUp.Dot(eye);

        res.m[2] = -forward.x;
        res.m[6] = -forward.y;
        res.m[10] = -forward.z;
        res.m[14] = forward.Dot(eye);

        res.m[3] = 0.0f;
        res.m[7] = 0.0f;
        res.m[11] = 0.0f;
        res.m[15] = 1.0f;
        return res;
    }
};

struct AABB {
    Vec3 min{1e9f, 1e9f, 1e9f};
    Vec3 max{-1e9f, -1e9f, -1e9f};

    void Expand(const Vec3& p) {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }

    Vec3 Center() const { return (min + max) * 0.5f; }
    Vec3 Size() const { return max - min; }
    float Radius() const { return Size().Length() * 0.5f; }
};

struct Ray {
    Vec3 origin;
    Vec3 direction; // Normalized

    bool IntersectAABB(const AABB& box, float& tMin, float& tMax) const {
        float t0 = (box.min.x - origin.x) / (std::abs(direction.x) > 1e-6f ? direction.x : 1e-6f);
        float t1 = (box.max.x - origin.x) / (std::abs(direction.x) > 1e-6f ? direction.x : 1e-6f);
        if (t0 > t1) std::swap(t0, t1);

        float ty0 = (box.min.y - origin.y) / (std::abs(direction.y) > 1e-6f ? direction.y : 1e-6f);
        float ty1 = (box.max.y - origin.y) / (std::abs(direction.y) > 1e-6f ? direction.y : 1e-6f);
        if (ty0 > ty1) std::swap(ty0, ty1);

        if ((t0 > ty1) || (ty0 > t1)) return false;
        if (ty0 > t0) t0 = ty0;
        if (ty1 < t1) t1 = ty1;

        float tz0 = (box.min.z - origin.z) / (std::abs(direction.z) > 1e-6f ? direction.z : 1e-6f);
        float tz1 = (box.max.z - origin.z) / (std::abs(direction.z) > 1e-6f ? direction.z : 1e-6f);
        if (tz0 > tz1) std::swap(tz0, tz1);

        if ((t0 > tz1) || (tz0 > t1)) return false;
        if (tz0 > t0) t0 = tz0;
        if (tz1 < t1) t1 = tz1;

        tMin = t0;
        tMax = t1;
        return tMax >= std::max(0.0f, tMin);
    }
};

} // namespace samp_editor
