// =============================================================================
// scene.cpp  —  Project 5: Front Desk Lobby Scene with Phong Lighting
//
// Improvements over Project 4:
//   - Per-face normals on all geometry (position + normal per vertex)
//   - Phong lighting model: ambient + diffuse + specular per fragment
//   - Per-object material properties (specularStrength, shininess)
//   - Cylinder objects (pendant lights, chair columns) also lit
//   - Floor tile grid drawn with line geometry for visual texture
//   - Two light sources: overhead warm white + cooler fill from the front
//
// Controls:
//   W/S/A/D     — move camera
//   Arrow keys  — rotate camera
//   ESC         — quit
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

const int   WIN_W = 1000;
const int   WIN_H = 700;
const char* TITLE = "Project 5 — Front Desk Lobby (Phong Lighting)  [WASD + Arrows]";

// -----------------------------------------------------------------------------
// Vertex shader: transforms position and normal into world space,
// passes both to the fragment stage for per-fragment lighting.
// -----------------------------------------------------------------------------
const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMatrix;

out vec3 vFragPos;
out vec3 vNormal;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos  = worldPos.xyz;
    vNormal   = normalize(uNormalMatrix * aNormal);
    gl_Position = uProj * uView * worldPos;
}
)";

// -----------------------------------------------------------------------------
// Fragment shader: Phong lighting with two point lights.
// Ambient keeps faces from going pure black.
// Diffuse (Lambertian N.L) provides the main shape cue.
// Specular (Blinn-Phong halfway vector) adds surface material feel.
// -----------------------------------------------------------------------------
const char* FRAG_SRC = R"(
#version 330 core
in vec3 vFragPos;
in vec3 vNormal;

uniform vec3  uColor;
uniform vec3  uLightPos;
uniform vec3  uLightColor;
uniform vec3  uLightPos2;
uniform vec3  uLightColor2;
uniform vec3  uViewPos;
uniform float uSpecularStrength;
uniform float uShininess;
uniform float uAmbientStrength;

out vec4 FragColor;

vec3 calcLight(vec3 lightPos, vec3 lightColor, vec3 N, vec3 V) {
    vec3 L    = normalize(lightPos - vFragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 H    = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uShininess);
    return diff * lightColor + uSpecularStrength * spec * lightColor;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vFragPos);

    vec3 ambient  = uAmbientStrength * (uLightColor + uLightColor2) * 0.5;
    vec3 lighting = calcLight(uLightPos,  uLightColor,  N, V)
                  + calcLight(uLightPos2, uLightColor2, N, V);

    FragColor = vec4((ambient + lighting) * uColor, 1.0);
}
)";

// -----------------------------------------------------------------------------
// Unit cube WITH per-face normals. Layout: pos(3) + normal(3) = 6 floats/vert.
// -----------------------------------------------------------------------------
const float CUBE_VERTS[] = {
    -0.5f,-0.5f,-0.5f, 0, 0,-1,  0.5f,-0.5f,-0.5f, 0, 0,-1,  0.5f, 0.5f,-0.5f, 0, 0,-1,
     0.5f, 0.5f,-0.5f, 0, 0,-1, -0.5f, 0.5f,-0.5f, 0, 0,-1, -0.5f,-0.5f,-0.5f, 0, 0,-1,
    -0.5f,-0.5f, 0.5f, 0, 0, 1,  0.5f,-0.5f, 0.5f, 0, 0, 1,  0.5f, 0.5f, 0.5f, 0, 0, 1,
     0.5f, 0.5f, 0.5f, 0, 0, 1, -0.5f, 0.5f, 0.5f, 0, 0, 1, -0.5f,-0.5f, 0.5f, 0, 0, 1,
    -0.5f, 0.5f, 0.5f,-1, 0, 0, -0.5f, 0.5f,-0.5f,-1, 0, 0, -0.5f,-0.5f,-0.5f,-1, 0, 0,
    -0.5f,-0.5f,-0.5f,-1, 0, 0, -0.5f,-0.5f, 0.5f,-1, 0, 0, -0.5f, 0.5f, 0.5f,-1, 0, 0,
     0.5f, 0.5f, 0.5f, 1, 0, 0,  0.5f, 0.5f,-0.5f, 1, 0, 0,  0.5f,-0.5f,-0.5f, 1, 0, 0,
     0.5f,-0.5f,-0.5f, 1, 0, 0,  0.5f,-0.5f, 0.5f, 1, 0, 0,  0.5f, 0.5f, 0.5f, 1, 0, 0,
    -0.5f,-0.5f,-0.5f, 0,-1, 0,  0.5f,-0.5f,-0.5f, 0,-1, 0,  0.5f,-0.5f, 0.5f, 0,-1, 0,
     0.5f,-0.5f, 0.5f, 0,-1, 0, -0.5f,-0.5f, 0.5f, 0,-1, 0, -0.5f,-0.5f,-0.5f, 0,-1, 0,
    -0.5f, 0.5f,-0.5f, 0, 1, 0,  0.5f, 0.5f,-0.5f, 0, 1, 0,  0.5f, 0.5f, 0.5f, 0, 1, 0,
     0.5f, 0.5f, 0.5f, 0, 1, 0, -0.5f, 0.5f, 0.5f, 0, 1, 0, -0.5f, 0.5f,-0.5f, 0, 1, 0,
};

// GPU handles
GLuint g_cubeVAO, g_cubeVBO;
GLuint g_cylVAO, g_cylVBO;
GLuint g_gridVAO, g_gridVBO;
GLuint g_prog;
int    g_cylVertCount = 0;
int    g_gridVertCount = 0;

// Camera
glm::vec3 g_camPos = glm::vec3(-2.0f, 1.6f, 5.0f);
glm::vec3 g_camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 g_camUp = glm::vec3(0.0f, 1.0f, 0.0f);
float     g_yaw = -75.0f;
float     g_pitch = -8.0f;

// Two light sources
const glm::vec3 LIGHT1_POS = glm::vec3(0.0f, 3.5f, 0.0f);    // overhead centre
const glm::vec3 LIGHT1_COLOR = glm::vec3(1.0f, 0.97f, 0.90f);   // warm white
const glm::vec3 LIGHT2_POS = glm::vec3(-3.0f, 2.0f, 5.0f);   // front fill
const glm::vec3 LIGHT2_COLOR = glm::vec3(0.55f, 0.60f, 0.70f);  // cool blue fill

// -----------------------------------------------------------------------------
// buildCylinder: smooth cylinder with per-vertex normals for lighting.
// -----------------------------------------------------------------------------
void buildCylinder(int seg = 32) {
    std::vector<float> v;
    float r = 0.5f, pi = 3.14159265f;
    for (int i = 0; i < seg; ++i) {
        float a0 = 2 * pi * i / seg, a1 = 2 * pi * (i + 1) / seg;
        float x0 = r * cosf(a0), z0 = r * sinf(a0);
        float x1 = r * cosf(a1), z1 = r * sinf(a1);
        float nx0 = cosf(a0), nz0 = sinf(a0);
        float nx1 = cosf(a1), nz1 = sinf(a1);
        // Side — normals point outward radially
        v.insert(v.end(), { x0,-0.5f,z0,nx0,0,nz0, x1,-0.5f,z1,nx1,0,nz1, x1,0.5f,z1,nx1,0,nz1 });
        v.insert(v.end(), { x0,-0.5f,z0,nx0,0,nz0, x1,0.5f,z1,nx1,0,nz1,  x0,0.5f,z0,nx0,0,nz0 });
        // Top cap — normal up
        v.insert(v.end(), { 0,0.5f,0,0,1,0, x0,0.5f,z0,0,1,0, x1,0.5f,z1,0,1,0 });
        // Bottom cap — normal down
        v.insert(v.end(), { 0,-0.5f,0,0,-1,0, x1,-0.5f,z1,0,-1,0, x0,-0.5f,z0,0,-1,0 });
    }
    g_cylVertCount = (int)v.size() / 6;
    glGenVertexArrays(1, &g_cylVAO); glGenBuffers(1, &g_cylVBO);
    glBindVertexArray(g_cylVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_cylVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// -----------------------------------------------------------------------------
// buildFloorGrid: a flat grid of lines to simulate tile seams on the floor.
// Normal points up (0,1,0) for all vertices.
// -----------------------------------------------------------------------------
void buildFloorGrid() {
    std::vector<float> v;
    int   lines = 16;
    float half = 4.0f;
    float step = (2.0f * half) / lines;
    for (int i = 0; i <= lines; ++i) {
        float t = -half + i * step;
        // Along X
        v.insert(v.end(), { -half, 0.001f, t, 0,1,0 });
        v.insert(v.end(), { half, 0.001f, t, 0,1,0 });
        // Along Z
        v.insert(v.end(), { t, 0.001f,-half, 0,1,0 });
        v.insert(v.end(), { t, 0.001f, half, 0,1,0 });
    }
    g_gridVertCount = (int)v.size() / 6;
    glGenVertexArrays(1, &g_gridVAO); glGenBuffers(1, &g_gridVBO);
    glBindVertexArray(g_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

GLuint compileShader(GLenum t, const char* s) {
    GLuint id = glCreateShader(t);
    glShaderSource(id, 1, &s, nullptr); glCompileShader(id);
    int ok; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[512]; glGetShaderInfoLog(id, 512, nullptr, l); std::cerr << l; }
    return id;
}
GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// -----------------------------------------------------------------------------
// setLightUniforms: uploads both lights + camera pos (called once per frame).
// -----------------------------------------------------------------------------
void setLightUniforms() {
    glUniform3fv(glGetUniformLocation(g_prog, "uLightPos"), 1, glm::value_ptr(LIGHT1_POS));
    glUniform3fv(glGetUniformLocation(g_prog, "uLightColor"), 1, glm::value_ptr(LIGHT1_COLOR));
    glUniform3fv(glGetUniformLocation(g_prog, "uLightPos2"), 1, glm::value_ptr(LIGHT2_POS));
    glUniform3fv(glGetUniformLocation(g_prog, "uLightColor2"), 1, glm::value_ptr(LIGHT2_COLOR));
    glUniform3fv(glGetUniformLocation(g_prog, "uViewPos"), 1, glm::value_ptr(g_camPos));
}

// -----------------------------------------------------------------------------
// drawBox: uploads model matrix + material, draws the cube.
// -----------------------------------------------------------------------------
void drawBox(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj,
    glm::vec3 color, float spec = 0.2f, float shine = 32.0f, float amb = 0.18f) {
    glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix3fv(glGetUniformLocation(g_prog, "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(nm));
    glUniform3fv(glGetUniformLocation(g_prog, "uColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(g_prog, "uSpecularStrength"), spec);
    glUniform1f(glGetUniformLocation(g_prog, "uShininess"), shine);
    glUniform1f(glGetUniformLocation(g_prog, "uAmbientStrength"), amb);
    glBindVertexArray(g_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// -----------------------------------------------------------------------------
// drawCylinder: same uniforms, draws the cylinder VAO.
// -----------------------------------------------------------------------------
void drawCylinder(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj,
    glm::vec3 color, float spec = 0.2f, float shine = 32.0f, float amb = 0.18f) {
    glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix3fv(glGetUniformLocation(g_prog, "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(nm));
    glUniform3fv(glGetUniformLocation(g_prog, "uColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(g_prog, "uSpecularStrength"), spec);
    glUniform1f(glGetUniformLocation(g_prog, "uShininess"), shine);
    glUniform1f(glGetUniformLocation(g_prog, "uAmbientStrength"), amb);
    glBindVertexArray(g_cylVAO);
    glDrawArrays(GL_TRIANGLES, 0, g_cylVertCount);
}

// makeModel helper
glm::mat4 makeModel(glm::vec3 sc, glm::vec3 tr, float rotYDeg = 0.0f) {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, tr);
    if (rotYDeg != 0.0f) m = glm::rotate(m, glm::radians(rotYDeg), glm::vec3(0, 1, 0));
    m = glm::scale(m, sc);
    return m;
}

void drawChair(float cx, float cz, const glm::mat4& v, const glm::mat4& p) {
    drawBox(makeModel({ 0.5f,0.06f,0.5f }, { cx,0.55f,cz }), v, p, { 0.08f,0.08f,0.08f }, 0.3f, 48.0f);
    drawBox(makeModel({ 0.48f,0.5f,0.06f }, { cx,0.85f,cz - 0.22f }), v, p, { 0.08f,0.08f,0.08f }, 0.3f, 48.0f);
    drawCylinder(makeModel({ 0.06f,0.55f,0.06f }, { cx,0.27f,cz }), v, p, { 0.15f,0.15f,0.15f }, 0.5f, 64.0f);
    drawCylinder(makeModel({ 0.45f,0.04f,0.45f }, { cx,0.04f,cz }), v, p, { 0.15f,0.15f,0.15f }, 0.5f, 64.0f);
}

void drawPendant(float px, float pz, const glm::mat4& v, const glm::mat4& p) {
    drawCylinder(makeModel({ 0.02f,0.8f,0.02f }, { px,2.8f,pz }), v, p, { 0.1f,0.1f,0.1f }, 0.1f, 16.0f);
    drawCylinder(makeModel({ 0.12f,0.4f,0.12f }, { px,2.38f,pz }), v, p, { 0.08f,0.08f,0.08f }, 0.6f, 96.0f, 0.1f);
}

// -----------------------------------------------------------------------------
// drawScene: all 13+ objects with Phong material parameters.
// -----------------------------------------------------------------------------
void drawScene(const glm::mat4& view, const glm::mat4& proj) {
    const float CX = 1.2f, CZ = -1.0f;

    // Floor — matte tile, very low specular
    drawBox(makeModel({ 8.0f,0.05f,7.0f }, { 0,-0.025f,0 }), view, proj,
        { 0.72f,0.70f,0.68f }, 0.08f, 16.0f, 0.20f);

    // Floor tile grid lines (slightly darker grey)
    {
        glm::mat4 gm = glm::mat4(1.0f);
        glm::mat3 nm = glm::mat3(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(g_prog, "uModel"), 1, GL_FALSE, glm::value_ptr(gm));
        glUniformMatrix4fv(glGetUniformLocation(g_prog, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(g_prog, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix3fv(glGetUniformLocation(g_prog, "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(nm));
        glUniform3f(glGetUniformLocation(g_prog, "uColor"), 0.58f, 0.56f, 0.54f);
        glUniform1f(glGetUniformLocation(g_prog, "uSpecularStrength"), 0.0f);
        glUniform1f(glGetUniformLocation(g_prog, "uShininess"), 1.0f);
        glUniform1f(glGetUniformLocation(g_prog, "uAmbientStrength"), 0.25f);
        glBindVertexArray(g_gridVAO);
        glDrawArrays(GL_LINES, 0, g_gridVertCount);
    }

    // Ceiling — off-white, low gloss
    drawBox(makeModel({ 8.0f,0.1f,7.0f }, { 0,3.2f,0 }), view, proj,
        { 0.90f,0.90f,0.88f }, 0.05f, 8.0f, 0.20f);

    // Back wall — matte paint
    drawBox(makeModel({ 8.0f,3.2f,0.1f }, { 0,1.55f,-3.5f }), view, proj,
        { 0.72f,0.71f,0.68f }, 0.03f, 8.0f, 0.12f);

    // Left side wall
    drawBox(makeModel({ 0.1f,3.2f,7.0f }, { -4.0f,1.6f,0 }), view, proj,
        { 0.86f,0.85f,0.82f }, 0.05f, 8.0f, 0.20f);

    // Yellow counter front panel — painted laminate, matte
    drawBox(makeModel({ 2.4f,1.2f,0.3f }, { CX,0.55f,CZ }), view, proj,
        { 0.95f,0.78f,0.06f }, 0.15f, 24.0f, 0.18f);

    // Counter base
    drawBox(makeModel({ 2.4f,0.6f,0.5f }, { CX,-0.1f,CZ }), view, proj,
        { 0.08f,0.08f,0.08f }, 0.25f, 32.0f, 0.15f);

    // Counter left side panel
    drawBox(makeModel({ 0.2f,1.2f,0.5f }, { CX - 1.3f,0.3f,CZ }), view, proj,
        { 0.08f,0.08f,0.08f }, 0.25f, 32.0f, 0.15f);

    // Counter right side panel
    drawBox(makeModel({ 0.2f,1.2f,0.5f }, { CX + 1.3f,0.3f,CZ }), view, proj,
        { 0.08f,0.08f,0.08f }, 0.25f, 32.0f, 0.15f);

    // Work desk tabletop — dark wood, moderate sheen
    drawBox(makeModel({ 1.6f,0.06f,0.9f }, { -1.0f,0.76f,-1.0f }), view, proj,
        { 0.22f,0.16f,0.12f }, 0.30f, 40.0f, 0.18f);

    // Desk left leg
    drawBox(makeModel({ 0.1f,0.8f,0.85f }, { -1.7f,0.38f,-1.0f }), view, proj,
        { 0.18f,0.13f,0.10f }, 0.20f, 32.0f, 0.15f);

    // Desk right leg
    drawBox(makeModel({ 0.1f,0.8f,0.85f }, { -0.3f,0.38f,-1.0f }), view, proj,
        { 0.18f,0.13f,0.10f }, 0.20f, 32.0f, 0.15f);

    // TV bezel — black plastic, moderate gloss
    drawBox(makeModel({ 1.2f,0.72f,0.06f }, { 1.0f,1.9f,-3.44f }), view, proj,
        { 0.08f,0.08f,0.08f }, 0.40f, 64.0f, 0.12f);

    // TV screen — blue, high specular (glass-like)
    drawBox(makeModel({ 1.1f,0.62f,0.04f }, { 1.0f,1.9f,-3.40f }), view, proj,
        { 0.20f,0.35f,0.70f }, 0.70f, 128.0f, 0.15f);

    // Whiteboard — near-white matte board surface
    drawBox(makeModel({ 0.7f,1.2f,0.04f }, { 2.2f,1.7f,-3.4f }), view, proj,
        { 0.95f,0.95f,0.95f }, 0.10f, 16.0f, 0.25f);

    // Pendant lights x2
    drawPendant(CX - 0.9f, CZ - 0.8f, view, proj);
    drawPendant(CX + 0.3f, CZ - 0.8f, view, proj);

    // Office chairs x2
    drawChair(-0.73f, -1.4f, view, proj);
    drawChair(-1.3f, -1.4f, view, proj);

    // Entrance door frame — dark metal
    drawBox(makeModel({ 2.0f,2.6f,0.06f }, { 4.0f,1.3f,0.5f }, 90.0f), view, proj,
        { 0.10f,0.10f,0.10f }, 0.50f, 80.0f, 0.12f);

    // Door glass panels — blue-grey, high specular
    drawBox(makeModel({ 0.8f,2.55f,0.1f }, { 4.0f,1.3f,0.0f }, 90.0f), view, proj,
        { 0.50f,0.65f,0.75f }, 0.80f, 128.0f, 0.15f);
    drawBox(makeModel({ 0.8f,2.55f,0.1f }, { 4.0f,1.3f,1.0f }, 90.0f), view, proj,
        { 0.50f,0.65f,0.75f }, 0.80f, 128.0f, 0.15f);
}

void framebufferSizeCallback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }
void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(win, true);
}

void updateCamera() {
    glm::vec3 f;
    f.x = cosf(glm::radians(g_yaw)) * cosf(glm::radians(g_pitch));
    f.y = sinf(glm::radians(g_pitch));
    f.z = sinf(glm::radians(g_yaw)) * cosf(glm::radians(g_pitch));
    g_camFront = glm::normalize(f);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* win = glfwCreateWindow(WIN_W, WIN_H, TITLE, nullptr, nullptr);
    if (!win) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);
    glfwSetKeyCallback(win, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    g_prog = createProgram();

    glGenVertexArrays(1, &g_cubeVAO); glGenBuffers(1, &g_cubeVBO);
    glBindVertexArray(g_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    buildCylinder(32);
    buildFloorGrid();

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)WIN_W / WIN_H, 0.1f, 100.0f);

    float moveSpeed = 2.5f, turnSpeed = 60.0f;
    double lastTime = glfwGetTime();
    updateCamera();

    glClearColor(0.55f, 0.55f, 0.58f, 1.0f);
    std::cout << "WASD = move  |  Arrow keys = look  |  ESC = quit\n";

    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime); lastTime = now;
        glfwPollEvents();

        float ms = moveSpeed * dt, ts = turnSpeed * dt;
        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) g_camPos += ms * g_camFront;
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) g_camPos -= ms * g_camFront;
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
            g_camPos -= glm::normalize(glm::cross(g_camFront, g_camUp)) * ms;
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
            g_camPos += glm::normalize(glm::cross(g_camFront, g_camUp)) * ms;
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) g_yaw -= ts;
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) g_yaw += ts;
        if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS) g_pitch += ts;
        if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) g_pitch -= ts;
        g_pitch = glm::clamp(g_pitch, -89.0f, 89.0f);
        updateCamera();

        glm::mat4 view = glm::lookAt(g_camPos, g_camPos + g_camFront, g_camUp);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_prog);
        setLightUniforms();
        drawScene(view, proj);
        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1, &g_cubeVAO); glDeleteBuffers(1, &g_cubeVBO);
    glDeleteVertexArrays(1, &g_cylVAO);  glDeleteBuffers(1, &g_cylVBO);
    glDeleteVertexArrays(1, &g_gridVAO); glDeleteBuffers(1, &g_gridVBO);
    glDeleteProgram(g_prog);
    glfwTerminate();
    return 0;
}