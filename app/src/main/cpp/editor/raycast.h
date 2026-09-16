#pragma once

#include "../core/types.h"

namespace samp_editor {

class RaycastUtil {
public:
    static Ray ScreenPointToRay(float screenX, float screenY, float screenWidth, float screenHeight,
                                const Mat4& viewMatrix, const Mat4& projMatrix);

    static bool InvertMatrix(const Mat4& in, Mat4& out);
};

} // namespace samp_editor
