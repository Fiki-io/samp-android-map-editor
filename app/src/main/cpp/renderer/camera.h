#pragma once

#include "../core/types.h"

namespace samp_editor {

class Camera {
public:
    Camera();

    void SetPosition(const Vec3& pos) { m_Position = pos; }
    const Vec3& GetPosition() const { return m_Position; }

    void SetRotation(float yawDeg, float pitchDeg);
    float GetYaw() const { return m_Yaw; }
    float GetPitch() const { return m_Pitch; }

    void Move(const Vec3& localDirection, float deltaTime);
    void Rotate(float deltaYaw, float deltaPitch);
    void Zoom(float deltaDistance);

    Vec3 GetForward() const { return m_Forward; }
    Vec3 GetRight() const { return m_Right; }
    Vec3 GetUp() const { return m_Up; }

    Mat4 GetViewMatrix() const;
    Mat4 GetProjectionMatrix(float aspect, float nearZ = 0.5f, float farZ = 1500.0f) const;

    float moveSpeed{25.0f};
    float sensitivity{0.2f};
    float fovDeg{65.0f};

private:
    void UpdateVectors();

    Vec3 m_Position{0.0f, 0.0f, 20.0f};
    Vec3 m_Forward{1.0f, 0.0f, 0.0f};
    Vec3 m_Right{0.0f, 1.0f, 0.0f};
    Vec3 m_Up{0.0f, 0.0f, 1.0f};

    float m_Yaw{0.0f};   // Rotasi Z
    float m_Pitch{0.0f}; // Rotasi kemiringan vertikal
};

} // namespace samp_editor
