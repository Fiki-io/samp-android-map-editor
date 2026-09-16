#include "gizmo.h"
#include <algorithm>
#include <cmath>

namespace samp_editor {

TransformGizmo::TransformGizmo() {}

void TransformGizmo::SetTarget(EditorObject* target) {
    m_Target = target;
    m_ActiveAxis = GizmoAxis::None;
}

Vec3 TransformGizmo::GetPosition() const {
    if (m_Target) {
        return m_Target->position;
    }
    return {0, 0, 0};
}

float TransformGizmo::ClosestDistanceRaySegment(const Ray& ray, const Vec3& segA, const Vec3& segB, Vec3& outClosestOnSeg) {
    Vec3 u = ray.direction;
    Vec3 v = segB - segA;
    Vec3 w = ray.origin - segA;

    float a = u.Dot(u);
    float b = u.Dot(v);
    float c = v.Dot(v);
    float d = u.Dot(w);
    float e = v.Dot(w);

    float D = a * c - b * b;
    float sc, sN, sD = D;
    float tc, tN, tD = D;

    if (D < 1e-6f) {
        sN = 0.0f;
        sD = 1.0f;
        tN = e;
        tD = c;
    } else {
        sN = (b * e - c * d);
        tN = (a * e - b * d);
        if (sN < 0.0f) {
            sN = 0.0f;
            tN = e;
            tD = c;
        }
    }

    if (tN < 0.0f) {
        tc = 0.0f;
    } else if (tN > tD) {
        tc = 1.0f;
    } else {
        tc = tN / tD;
    }

    sc = (std::abs(sN) < 1e-6f ? 0.0f : sN / sD);

    Vec3 ptOnRay = ray.origin + u * sc;
    outClosestOnSeg = segA + v * tc;

    return (ptOnRay - outClosestOnSeg).Length();
}

GizmoAxis TransformGizmo::TestHit(const Ray& ray, float axisLength, float hitThreshold) {
    if (!m_Target) return GizmoAxis::None;

    Vec3 pos = m_Target->position;
    Vec3 segX = pos + Vec3(axisLength, 0, 0);
    Vec3 segY = pos + Vec3(0, axisLength, 0);
    Vec3 segZ = pos + Vec3(0, 0, axisLength);

    Vec3 closestPt;
    float distX = ClosestDistanceRaySegment(ray, pos, segX, closestPt);
    float distY = ClosestDistanceRaySegment(ray, pos, segY, closestPt);
    float distZ = ClosestDistanceRaySegment(ray, pos, segZ, closestPt);

    float minDist = hitThreshold;
    GizmoAxis chosen = GizmoAxis::None;

    if (distX < minDist) {
        minDist = distX;
        chosen = GizmoAxis::X;
    }
    if (distY < minDist) {
        minDist = distY;
        chosen = GizmoAxis::Y;
    }
    if (distZ < minDist) {
        minDist = distZ;
        chosen = GizmoAxis::Z;
    }

    return chosen;
}

void TransformGizmo::BeginDrag(GizmoAxis axis, const Vec3& hitPoint) {
    if (!m_Target) return;
    m_ActiveAxis = axis;
    m_InitialObjPos = m_Target->position;
    m_DragStartHit = hitPoint;
}

void TransformGizmo::UpdateDrag(const Ray& ray) {
    if (!m_Target || m_ActiveAxis == GizmoAxis::None) return;

    Vec3 axisDir{0, 0, 0};
    if (m_ActiveAxis == GizmoAxis::X) axisDir = {1, 0, 0};
    else if (m_ActiveAxis == GizmoAxis::Y) axisDir = {0, 1, 0};
    else if (m_ActiveAxis == GizmoAxis::Z) axisDir = {0, 0, 1};

    if (m_Mode == GizmoMode::Translate) {
        // Cari titik terdekat pada sumbu
        Vec3 closestOnAxis;
        ClosestDistanceRaySegment(ray, m_InitialObjPos - axisDir * 100.0f, m_InitialObjPos + axisDir * 100.0f, closestOnAxis);

        float projDist = (closestOnAxis - m_InitialObjPos).Dot(axisDir);
        m_Target->position = m_InitialObjPos + axisDir * projDist;
    } else if (m_Mode == GizmoMode::Rotate) {
        // Putar Euler angle
        if (m_ActiveAxis == GizmoAxis::X) m_Target->rotationEuler.x += 1.0f;
        else if (m_ActiveAxis == GizmoAxis::Y) m_Target->rotationEuler.y += 1.0f;
        else if (m_ActiveAxis == GizmoAxis::Z) m_Target->rotationEuler.z += 1.0f;
        m_Target->SyncQuatFromEuler();
    }
}

void TransformGizmo::EndDrag() {
    m_ActiveAxis = GizmoAxis::None;
}

} // namespace samp_editor
