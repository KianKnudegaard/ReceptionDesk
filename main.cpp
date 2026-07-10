// =============================================================================
// ReceptionDesk · main.cpp  (Project 4 + Phong lighting extension)
//
// Discussion prompt: How would you apply a lighting model to your scene?
//
// Scene object: Apartment building front desk reception counter
// Components:
//   1. Front Panel  — tall yellow facing (wide, tall, thin box)
//   2. Base Plinth  — dark strip at floor level
//   3. Side Panels  — dark blocks flanking the yellow facing
//
// LIGHTING MODEL: Phong (ambient + diffuse + specular), per-fragment.
//   - Ambient:  constant 0.15 * objectColor, so no face goes pure black
//   - Diffuse:  Lambertian N.L term, clamped to 0 (see Topic 5 activity)
//   - Specular: Blinn-Phong halfway vector, low strength/shininess for
//               matte laminate/painted surfaces (no glossy highlight)
//
// Each component is a unit cube (now with per-face normals) transformed by:
//   Scale       → stretch into correct proportions
//   Translate   → position in world space
//   Rotate      → Y-axis rotation for interactive viewing
//
// Controls:
//   LEFT / RIGHT arrow  — rotate the counter
//   ESC                 — quit
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// -----------------------------------------------------------------------------
// Window settings
// -----------------------------------------------------------------------------
const int   WIN_W = 900;
const int   WIN_H = 700;
const char* WIN_TITLE = "Reception Desk - Lab: Transformations + Lighting";

// -----------------------------------------------------------------------------
// GLSL shaders (inline - no external files needed)
//
// Vertex shader:   transforms position AND normal by model/view/proj,
//                  passes world-space position + normal to the fragment stage
// Fragment shader: computes Phong lighting (ambient + diffuse + specular)
// -----------------------------------------------------------------------------
const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMatrix; // transpose(inverse(mat3(model))), handles non-uniform scale

out vec3 vFragPos;
out vec3 vNormal;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    gl_Position = uProj * uView * worldPos;
}
)";

const char* FRAG_SRC = R"(
#version 330 core
in vec3 vFragPos;
in vec3 vNormal;

uniform vec3 uColor;         // object's base/solid color
uniform vec3 uLightPos;      // world-space light position
uniform vec3 uLightColor;    // light's color/intensity
uniform vec3 uViewPos;       // camera position (for specular)
uniform float uSpecularStrength;
uniform float uShininess;

out vec4 FragColor;

void main() {
    // Ambient: keeps unlit faces from going pure black
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * uLightColor;

    // Diffuse: Lambertian N.L, clamped to 0 (the "trap" case from Topic 5)
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vFragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Specular: Blinn-Phong halfway vector, kept subtle for matte surfaces
    vec3 V = normalize(uViewPos - vFragPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uShininess);
    vec3 specular = uSpecularStrength * spec * uLightColor;

    vec3 result = (ambient + diffuse + specular) * uColor;
    FragColor = vec4(result, 1.0);
}
)";

// -----------------------------------------------------------------------------
// Unit cube vertices (1x1x1 centred at origin), now WITH per-face normals.
// Layout per vertex: position(3) + normal(3) = 6 floats.
// 36 vertices / 12 triangles, same winding as the original.
// -----------------------------------------------------------------------------
float g_cubeVerts[] = {
    // Back face   (normal 0,0,-1)
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
     0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,

    // Front face  (normal 0,0,1)
    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
     0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,

    // Left face   (normal -1,0,0)
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,

    // Right face  (normal 1,0,0)
     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, 0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,-0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,

     // Bottom face (normal 0,-1,0)
     -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
      0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
      0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
      0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
     -0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
     -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,

     // Top face    (normal 0,1,0)
     -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
      0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
      0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
      0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
     -0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
     -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
};

// -----------------------------------------------------------------------------
// Global state
// -----------------------------------------------------------------------------
float g_angle = 0.3f;   // Y-axis rotation (arrow keys)
GLuint g_VAO, g_VBO;
GLuint g_prog;

// Camera position kept as a global so the fragment shader can use it for
// specular (view-dependent) lighting. Must match the eye passed to glm::lookAt.
glm::vec3 g_cameraPos = glm::vec3(-2.0f, 1.2f, 4.5f);

// A single point light placed above and in front of the desk, roughly where
// an overhead lobby fixture would sit.
glm::vec3 g_lightPos = glm::vec3(1.5f, 3.0f, 2.5f);
glm::vec3 g_lightColor = glm::vec3(1.0f, 1.0f, 0.95f); // slightly warm white

// -----------------------------------------------------------------------------
// Shader helpers
// -----------------------------------------------------------------------------
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return s;
}

GLuint createShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

// -----------------------------------------------------------------------------
// drawBox: draws the unit cube with a given model matrix, solid color, and
// material response (specular strength + shininess).
//
// Parameters:
//   model             — the model matrix (scale + translate + rotate combined)
//   view, proj        — camera matrices (shared across all objects)
//   color             — RGB base color for this object
//   specularStrength  — 0 = fully matte, higher = more visible highlight
//   shininess         — higher = tighter/sharper highlight
// -----------------------------------------------------------------------------
void drawBox(const glm::mat4& model,
    const glm::mat4& view,
    const glm::mat4& proj,
    glm::vec3 color,
    float specularStrength = 0.2f,
    float shininess = 32.0f)
{
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix3fv(glGetUniformLocation(g_prog, "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    glUniform3fv(glGetUniformLocation(g_prog, "uColor"), 1, glm::value_ptr(color));
    glUniform3fv(glGetUniformLocation(g_prog, "uLightPos"), 1, glm::value_ptr(g_lightPos));
    glUniform3fv(glGetUniformLocation(g_prog, "uLightColor"), 1, glm::value_ptr(g_lightColor));
    glUniform3fv(glGetUniformLocation(g_prog, "uViewPos"), 1, glm::value_ptr(g_cameraPos));
    glUniform1f(glGetUniformLocation(g_prog, "uSpecularStrength"), specularStrength);
    glUniform1f(glGetUniformLocation(g_prog, "uShininess"), shininess);

    glBindVertexArray(g_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// -----------------------------------------------------------------------------
// drawScene: builds and draws all objects in the scene.
// -----------------------------------------------------------------------------
void drawScene(const glm::mat4& view, const glm::mat4& proj) {

    // Shared Y-axis rotation applied to the entire counter assembly
    // so the user can inspect it from different angles with arrow keys.
    glm::mat4 rot = glm::rotate(
        glm::mat4(1.0f),
        g_angle,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // COMPONENT 1: Front Panel (yellow facing) - painted laminate, matte
    glm::mat4 panelModel = rot
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.55f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(2.4f, 1.1f, 0.4f));
    drawBox(panelModel, view, proj,
        glm::vec3(0.95f, 0.78f, 0.06f),   // yellow
        0.15f, 24.0f);                     // low specular, matte paint

    // COMPONENT 2: Base panel - black block underneath yellow panel
    glm::mat4 baseModel = rot
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(2.4f, 0.2f, 0.5f));
    drawBox(baseModel, view, proj,
        glm::vec3(0.08f, 0.08f, 0.08f),   // black
        0.25f, 32.0f);                     // slightly more sheen, painted MDF trim

    // COMPONENT 3: Side panel - black block on left of yellow panel
    glm::mat4 sidePanelModel = rot
        * glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.3f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 1.0f, 0.5f));
    drawBox(sidePanelModel, view, proj,
        glm::vec3(0.08f, 0.08f, 0.08f),   // black
        0.25f, 32.0f);

    // COMPONENT 4: Side panel - black block on right of yellow panel
    glm::mat4 sidePanelModel1 = rot
        * glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 0.3f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 1.0f, 0.5f));
    drawBox(sidePanelModel1, view, proj,
        glm::vec3(0.08f, 0.08f, 0.08f),   // black
        0.25f, 32.0f);

    // =========================================================================
    // To add more objects: call drawBox() with a new model matrix and, if the
    // material should look glossier or more matte, adjust the specular
    // strength / shininess args (e.g., closet glass would want a high
    // specular strength and high shininess for a tight, bright highlight).
    // =========================================================================
}

// -----------------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------------
void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main() {
    // -- GLFW init --
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 0);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, WIN_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    // -- GLAD init --
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // -- Shaders --
    g_prog = createShaderProgram();

    // -- Upload unit cube (position + normal) to GPU --
    glGenVertexArrays(1, &g_VAO);
    glGenBuffers(1, &g_VBO);
    glBindVertexArray(g_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_cubeVerts), g_cubeVerts, GL_STATIC_DRAW);

    // position: location 0, 3 floats, stride 6 floats, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal: location 1, 3 floats, stride 6 floats, offset 3 floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // -- Camera (view + projection) --
    glm::mat4 view = glm::lookAt(
        g_cameraPos,                   // camera position (kept in sync with global)
        glm::vec3(0.0f, 0.3f, 0.0f),   // look-at point
        glm::vec3(0.0f, 1.0f, 0.0f)    // world up
    );
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        (float)WIN_W / (float)WIN_H,
        0.1f,
        100.0f
    );

    // -- Render loop --
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Arrow key rotation
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) g_angle -= 0.02f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_angle += 0.02f;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_prog);

        drawScene(view, proj);

        glfwSwapBuffers(window);
    }

    // -- Cleanup --
    glDeleteVertexArrays(1, &g_VAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteProgram(g_prog);
    glfwTerminate();
    return 0;
}