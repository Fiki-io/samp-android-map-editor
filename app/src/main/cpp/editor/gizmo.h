#pragma once

#include "../core/types.h"
#include "exporter.h"

namespace samp_editor {

enum class GizmoAxis {
    None,
    X, // Merah (Translasi X)
    Y, // Hijau (Translasi Y)
    Z  // Biru (Translasi Z)
};

enum class GizmoMode {
    Translate,
    Rotate
};

class TransformGizmo {
public:
    TransformGizmo();

    void SetTarget(EditorObject* target);
    EditorObject* GetTarget() const { return m_Target; }
    bool HasTarget() const { return m_Target != nullptr; }

    void SetMode(GizmoMode mode) { m_Mode = mode; }
    GizmoMode GetMode() const { return m_Mode; }

    GizmoAxis TestHit(const Ray& ray, float axisLength = 2.0f, float hitThreshold = 0.3f);
    void BeginDrag(GizmoAxis axis, const Vec3& hitPoint);
    void UpdateDrag(const Ray& ray);
    void EndDrag();
    bool IsDragging() const { return m_ActiveAxis != GizmoAxis::None; }

    GizmoAxis GetActiveAxis() const { return m_ActiveAxis; }
    Vec3 GetPosition() const;

private:
    static float ClosestDistanceRaySegment(const Ray& ray, const Vec3& segA, const Vec3& segB, Vec3& outClosestOnSeg);

    EditorObject* m_Target{nullptr};
    GizmoMode m_Mode{GizmoMode::Translate};
    GizmoAxis m_ActiveAxis{GizmoAxis::None};

    Vec3 m_InitialObjPos;
    Vec3 m_DragStartHit;
};

} // namespace samp_editor
