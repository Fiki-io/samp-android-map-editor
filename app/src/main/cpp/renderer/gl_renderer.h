#pragma once

#include "../core/types.h"
#include "camera.h"
#include "../editor/scene.h"
#include <GLES3/gl3.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace samp_editor {

struct GPUMesh {
    GLuint vao{0};
    GLuint vbo{0};
    GLuint ebo{0};
    GLsizei indexCount{0};
    std::string textureName;
};

class GLRenderer {
public:
    GLRenderer();
    ~GLRenderer();

    bool InitShaders(const std::string& modelVertSrc, const std::string& modelFragSrc,
                     const std::string& gridVertSrc, const std::string& gridFragSrc);

    void OnSurfaceChanged(int width, int height);
    void Render(Scene& scene);

    Camera& GetCamera() { return m_Camera; }
    const Camera& GetCamera() const { return m_Camera; }

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    GLuint GetOrCreateTexture(const std::string& texName, Scene& scene, const std::string& txdHint = "");

private:
    void InitGrid();
    void RenderGrid(const Mat4& view, const Mat4& proj);
    void RenderGizmo(const TransformGizmo& gizmo, const Mat4& view, const Mat4& proj);
    void RenderModel(const DFFModel& model, const Mat4& modelMatrix, Scene& scene,
                     const Mat4& view, const Mat4& proj, const Vec4& tint = {1, 1, 1, 1});

    GLuint CompileShader(GLenum type, const std::string& src);
    GLuint LinkProgram(GLuint vert, GLuint frag);

    int m_Width{1280};
    int m_Height{720};
    Camera m_Camera;

    GLuint m_ModelProgram{0};
    GLuint m_GridProgram{0};

    // Uniform locations for model shader
    GLint m_uModelLoc{-1};
    GLint m_uViewLoc{-1};
    GLint m_uProjLoc{-1};
    GLint m_uTexLoc{-1};
    GLint m_uHasTexLoc{-1};
    GLint m_uTintLoc{-1};
    GLint m_uLightDirLoc{-1};

    // Uniform locations for grid/gizmo shader
    GLint m_uGridModelLoc{-1};
    GLint m_uGridViewLoc{-1};
    GLint m_uGridProjLoc{-1};

    GLuint m_GridVAO{0};
    GLuint m_GridVBO{0};
    GLsizei m_GridVertexCount{0};

    GLuint m_GizmoVAO{0};
    GLuint m_GizmoVBO{0};

    // GPU cache
    std::unordered_map<std::string, std::vector<GPUMesh>> m_MeshCache;
    std::unordered_map<std::string, GLuint> m_TextureCache;
};

} // namespace samp_editor
