#include "camera.h"
#include <algorithm>
#include <cmath>

namespace samp_editor {

Camera::Camera() {
    UpdateVectors();
}

void Camera::SetRotation(float yawDeg, float pitchDeg) {
    m_Yaw = yawDeg;
    m_Pitch = std::clamp(pitchDeg, -89.0f, 89.0f);
    UpdateVectors();
}

void Camera::Rotate(float deltaYaw, float deltaPitch) {
    m_Yaw += deltaYaw * sensitivity;
    m_Pitch = std::clamp(m_Pitch + deltaPitch * sensitivity, -89.0f, 89.0f);
    UpdateVectors();
}

void Camera::Move(const Vec3& localDir, float deltaTime) {
    Vec3 velocity = (m_Forward * localDir.x + m_Right * localDir.y + m_Up * localDir.z) * (moveSpeed * deltaTime);
    m_Position += velocity;
}

void Camera::Zoom(float deltaDistance) {
    m_Position += m_Forward * deltaDistance;
}

void Camera::UpdateVectors() {
    float yawRad = m_Yaw * DEG_TO_RAD;
    float pitchRad = m_Pitch * DEG_TO_RAD;

    // GTA SA: Z is Up, X is East, Y is North
    m_Forward.x = std::cos(pitchRad) * std::cos(yawRad);
    m_Forward.y = std::cos(pitchRad) * std::sin(yawRad);
    m_Forward.z = std::sin(pitchRad);
    m_Forward = m_Forward.Normalized();

    Vec3 worldUp = {0.0f, 0.0f, 1.0f};
    m_Right = m_Forward.Cross(worldUp).Normalized();
    m_Up = m_Right.Cross(m_Forward).Normalized();
}

Mat4 Camera::GetViewMatrix() const {
    return Mat4::LookAtGTA(m_Position, m_Position + m_Forward, m_Up);
}

Mat4 Camera::GetProjectionMatrix(float aspect, float nearZ, float farZ) const {
    return Mat4::Perspective(fovDeg * DEG_TO_RAD, aspect, nearZ, farZ);
}

} // namespace samp_editor
