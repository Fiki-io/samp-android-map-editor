#include "gl_renderer.h"
#include <iostream>
#include <cmath>

namespace samp_editor {

namespace {
    struct GridVertex {
        float x, y, z;
        float r, g, b, a;
    };
}

GLRenderer::GLRenderer() {
    m_Camera.SetPosition({0.0f, 0.0f, 30.0f});
    m_Camera.SetRotation(0.0f, -20.0f);
}

GLRenderer::~GLRenderer() {
    if (m_ModelProgram) glDeleteProgram(m_ModelProgram);
    if (m_GridProgram) glDeleteProgram(m_GridProgram);
    if (m_GridVAO) glDeleteVertexArrays(1, &m_GridVAO);
    if (m_GridVBO) glDeleteBuffers(1, &m_GridVBO);
    if (m_GizmoVAO) glDeleteVertexArrays(1, &m_GizmoVAO);
    if (m_GizmoVBO) glDeleteBuffers(1, &m_GizmoVBO);

    for (auto& [name, meshes] : m_MeshCache) {
        for (auto& m : meshes) {
            if (m.vao) glDeleteVertexArrays(1, &m.vao);
            if (m.vbo) glDeleteBuffers(1, &m.vbo);
            if (m.ebo) glDeleteBuffers(1, &m.ebo);
        }
    }

    for (auto& [name, tex] : m_TextureCache) {
        if (tex) glDeleteTextures(1, &tex);
    }
}

GLuint GLRenderer::CompileShader(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[GLRenderer] Shader Compilation Error:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint GLRenderer::LinkProgram(GLuint vert, GLuint frag) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "[GLRenderer] Program Link Error:\n" << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

bool GLRenderer::InitShaders(const std::string& modelVertSrc, const std::string& modelFragSrc,
                            const std::string& gridVertSrc, const std::string& gridFragSrc) {
    // 1. Model shader
    GLuint mV = CompileShader(GL_VERTEX_SHADER, modelVertSrc);
    GLuint mF = CompileShader(GL_FRAGMENT_SHADER, modelFragSrc);
    if (!mV || !mF) return false;
    m_ModelProgram = LinkProgram(mV, mF);
    glDeleteShader(mV);
    glDeleteShader(mF);

    m_uModelLoc = glGetUniformLocation(m_ModelProgram, "uModel");
    m_uViewLoc = glGetUniformLocation(m_ModelProgram, "uView");
    m_uProjLoc = glGetUniformLocation(m_ModelProgram, "uProjection");
    m_uTexLoc = glGetUniformLocation(m_ModelProgram, "uTexture");
    m_uHasTexLoc = glGetUniformLocation(m_ModelProgram, "uHasTexture");
    m_uTintLoc = glGetUniformLocation(m_ModelProgram, "uTint");
    m_uLightDirLoc = glGetUniformLocation(m_ModelProgram, "uLightDir");

    // 2. Grid & Gizmo shader
    GLuint gV = CompileShader(GL_VERTEX_SHADER, gridVertSrc);
    GLuint gF = CompileShader(GL_FRAGMENT_SHADER, gridFragSrc);
    if (!gV || !gF) return false;
    m_GridProgram = LinkProgram(gV, gF);
    glDeleteShader(gV);
    glDeleteShader(gF);

    m_uGridModelLoc = glGetUniformLocation(m_GridProgram, "uModel");
    m_uGridViewLoc = glGetUniformLocation(m_GridProgram, "uView");
    m_uGridProjLoc = glGetUniformLocation(m_GridProgram, "uProjection");

    InitGrid();
    return true;
}

void GLRenderer::InitGrid() {
    std::vector<GridVertex> vertices;
    float extent = 200.0f;
    float step = 5.0f;

    // Garis grid horizontal dan vertikal
    for (float x = -extent; x <= extent; x += step) {
        float alpha = (std::abs(x) < 0.1f) ? 0.9f : 0.3f;
        float r = (std::abs(x) < 0.1f) ? 0.8f : 0.4f;
        float g = (std::abs(x) < 0.1f) ? 0.2f : 0.4f;
        float b = 0.4f;

        vertices.push_back({x, -extent, 0.0f, r, g, b, alpha});
        vertices.push_back({x,  extent, 0.0f, r, g, b, alpha});
    }

    for (float y = -extent; y <= extent; y += step) {
        float alpha = (std::abs(y) < 0.1f) ? 0.9f : 0.3f;
        float r = 0.4f;
        float g = (std::abs(y) < 0.1f) ? 0.8f : 0.4f;
        float b = 0.4f;

        vertices.push_back({-extent, y, 0.0f, r, g, b, alpha});
        vertices.push_back({ extent, y, 0.0f, r, g, b, alpha});
    }

    m_GridVertexCount = static_cast<GLsizei>(vertices.size());

    glGenVertexArrays(1, &m_GridVAO);
    glGenBuffers(1, &m_GridVBO);

    glBindVertexArray(m_GridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GridVertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    // Setup Gizmo buffer
    glGenVertexArrays(1, &m_GizmoVAO);
    glGenBuffers(1, &m_GizmoVBO);
    glBindVertexArray(m_GizmoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_GizmoVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GridVertex), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}

void GLRenderer::OnSurfaceChanged(int width, int height) {
    m_Width = width > 0 ? width : 1;
    m_Height = height > 0 ? height : 1;
    glViewport(0, 0, m_Width, m_Height);
}

GLuint GLRenderer::GetOrCreateTexture(const std::string& texName, Scene& scene, const std::string& txdHint) {
    if (texName.empty()) return 0;

    auto it = m_TextureCache.find(texName);
    if (it != m_TextureCache.end()) {
        return it->second;
    }

    const DecodedTexture* dec = scene.GetOrLoadTexture(texName, txdHint);
    if (!dec || dec->rgbaPixels.empty()) {
        return 0;
    }

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dec->width, dec->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dec->rgbaPixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    m_TextureCache[texName] = texId;
    return texId;
}

void GLRenderer::Render(Scene& scene) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Sky blue background (GTA San Andreas daylight)
    glClearColor(0.42f, 0.65f, 0.88f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    Mat4 view = m_Camera.GetViewMatrix();
    Mat4 proj = m_Camera.GetProjectionMatrix(aspect);

    // 1. Render Ground Grid
    RenderGrid(view, proj);

    // 2. Query visible objects
    std::vector<EditorObject*> visibleEditorObjs;
    std::vector<const MapInstance*> visibleWorldObjs;
    scene.QueryVisibleObjects(m_Camera.GetPosition(), 400.0f, visibleEditorObjs, visibleWorldObjs);

    // 3. Render World Instances (Map asli San Andreas)
    for (const auto* inst : visibleWorldObjs) {
        if (!inst) continue;
        const DFFModel* model = scene.GetOrLoadModel(inst->modelName);
        if (model) {
            Mat4 modelMat = Mat4::Translation(inst->position) * Mat4::FromQuat(inst->rotation);
            RenderModel(*model, modelMat, scene, view, proj, {0.9f, 0.9f, 0.9f, 1.0f});
        }
    }

    // 4. Render User Editor Objects
    for (const auto* obj : visibleEditorObjs) {
        if (!obj) continue;
        const DFFModel* model = scene.GetOrLoadModel(obj->name);
        Mat4 modelMat = Mat4::Translation(obj->position) * Mat4::FromQuat(obj->rotationQuat);

        bool isSelected = (scene.GetGizmo().GetTarget() == obj);
        Vec4 tint = isSelected ? Vec4{1.2f, 1.2f, 0.5f, 1.0f} : Vec4{1.0f, 1.0f, 1.0f, 1.0f};

        if (model) {
            RenderModel(*model, modelMat, scene, view, proj, tint);
        }
    }

    // 5. Render 3D Transform Gizmo (jika ada objek terpilih)
    if (scene.GetGizmo().HasTarget()) {
        RenderGizmo(scene.GetGizmo(), view, proj);
    }
}

void GLRenderer::RenderGrid(const Mat4& view, const Mat4& proj) {
    if (!m_GridProgram || !m_GridVAO) return;

    glUseProgram(m_GridProgram);
    Mat4 identity;
    glUniformMatrix4fv(m_uGridModelLoc, 1, GL_FALSE, identity.m);
    glUniformMatrix4fv(m_uGridViewLoc, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(m_uGridProjLoc, 1, GL_FALSE, proj.m);

    glBindVertexArray(m_GridVAO);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, m_GridVertexCount);
    glBindVertexArray(0);
}

void GLRenderer::RenderGizmo(const TransformGizmo& gizmo, const Mat4& view, const Mat4& proj) {
    if (!m_GridProgram || !m_GizmoVAO) return;

    Vec3 pos = gizmo.GetPosition();
    float len = 3.5f;

    // 3 Sumbu: Merah (X), Hijau (Y), Biru (Z)
    GridVertex lines[6] = {
        {pos.x, pos.y, pos.z, 1.0f, 0.1f, 0.1f, 1.0f},
        {pos.x + len, pos.y, pos.z, 1.0f, 0.1f, 0.1f, 1.0f},

        {pos.x, pos.y, pos.z, 0.1f, 1.0f, 0.1f, 1.0f},
        {pos.x, pos.y + len, pos.z, 0.1f, 1.0f, 0.1f, 1.0f},

        {pos.x, pos.y, pos.z, 0.1f, 0.3f, 1.0f, 1.0f},
        {pos.x, pos.y, pos.z + len, 0.1f, 0.3f, 1.0f, 1.0f}
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_GizmoVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lines), lines);

    glUseProgram(m_GridProgram);
    Mat4 identity;
    glUniformMatrix4fv(m_uGridModelLoc, 1, GL_FALSE, identity.m);
    glUniformMatrix4fv(m_uGridViewLoc, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(m_uGridProjLoc, 1, GL_FALSE, proj.m);

    glDisable(GL_DEPTH_TEST); // Gizmo selalu terlihat di atas objek
    glBindVertexArray(m_GizmoVAO);
    glLineWidth(4.0f);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void GLRenderer::RenderModel(const DFFModel& model, const Mat4& modelMatrix, Scene& scene,
                            const Mat4& view, const Mat4& proj, const Vec4& tint) {
    if (!m_ModelProgram) return;

    glUseProgram(m_ModelProgram);
    glUniformMatrix4fv(m_uModelLoc, 1, GL_FALSE, modelMatrix.m);
    glUniformMatrix4fv(m_uViewLoc, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(m_uProjLoc, 1, GL_FALSE, proj.m);
    glUniform4f(m_uTintLoc, tint.x, tint.y, tint.z, tint.w);

    // Sun light direction
    Vec3 sunDir = Vec3(0.5f, -0.3f, 0.8f).Normalized();
    glUniform3f(m_uLightDirLoc, sunDir.x, sunDir.y, sunDir.z);

    // Cek atau buat GPU mesh cache
    auto& meshes = m_MeshCache[model.name];
    if (meshes.empty()) {
        for (const auto& geom : model.geometries) {
            for (const auto& sub : geom.subMeshes) {
                if (sub.indices.empty()) continue;

                GPUMesh gm;
                gm.textureName = sub.textureName;
                gm.indexCount = static_cast<GLsizei>(sub.indices.size());

                glGenVertexArrays(1, &gm.vao);
                glGenBuffers(1, &gm.vbo);
                glGenBuffers(1, &gm.ebo);

                glBindVertexArray(gm.vao);

                glBindBuffer(GL_ARRAY_BUFFER, gm.vbo);
                glBufferData(GL_ARRAY_BUFFER, geom.vertices.size() * sizeof(DFFVertex), geom.vertices.data(), GL_STATIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sub.indices.size() * sizeof(uint32_t), sub.indices.data(), GL_STATIC_DRAW);

                // Posisi (vec3)
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DFFVertex), (void*)offsetof(DFFVertex, position));

                // Normal (vec3)
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DFFVertex), (void*)offsetof(DFFVertex, normal));

                // UV (vec2)
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(DFFVertex), (void*)offsetof(DFFVertex, uv));

                // Color (vec4 RGBA)
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DFFVertex), (void*)offsetof(DFFVertex, color));

                glBindVertexArray(0);
                meshes.push_back(gm);
            }
        }
    }

    // Gambar setiap submesh
    for (const auto& gm : meshes) {
        GLuint tex = GetOrCreateTexture(gm.textureName, scene);
        if (tex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glUniform1i(m_uTexLoc, 0);
            glUniform1i(m_uHasTexLoc, 1);
        } else {
            glUniform1i(m_uHasTexLoc, 0);
        }

        glBindVertexArray(gm.vao);
        glDrawElements(GL_TRIANGLES, gm.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

} // namespace samp_editor
