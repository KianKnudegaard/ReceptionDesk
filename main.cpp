// =============================================================================
// ReceptionDesk · main.cpp
//
// Lab Question: Transformations applied to the reception desk front counter.
//
// Scene object: Apartment building front desk reception counter
// Components:
//   1. Front Panel  — tall yellow facing (wide, tall, thin box)
//   2. Counter Top  — flat grey work surface (wide, flat, deep box)
//   3. Base Plinth  — dark strip at floor level (wide, short, thin box)
//
// Each component is a unit cube transformed by:
//   Scale       → stretch into correct proportions
//   Translate   → position in world space
//   Rotate      → Y-axis rotation for interactive viewing
//
// Controls:
//   LEFT / RIGHT arrow  — rotate the counter
//   ESC                 — quit
//
// PROJECT 4 EXPANSION NOTES:
//   To add more objects to the scene, follow the pattern in drawScene():
//   create a model matrix with the desired transforms and call drawBox().
//   The camera, shaders, and VAO are all reusable as-is.
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
const char* WIN_TITLE = "Reception Desk — Lab: Transformations";

// -----------------------------------------------------------------------------
// GLSL shaders (inline — no external files needed)
//
// Vertex shader:   transforms each vertex by the MVP matrix
// Fragment shader: outputs a solid colour passed as a uniform
// -----------------------------------------------------------------------------
const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char* FRAG_SRC = R"(
#version 330 core
uniform vec3 uColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

// -----------------------------------------------------------------------------
// Unit cube vertices (1x1x1 centred at origin, 36 vertices / 12 triangles)
// All objects in the scene are built by transforming this same cube.
// -----------------------------------------------------------------------------
float g_cubeVerts[] = {
    // Back face
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
     0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
     // Front face
     -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
      0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
      // Left face
      -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
      -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
      // Right face
       0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
       0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
       // Bottom face
       -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
        0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
        // Top face
        -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
};

// -----------------------------------------------------------------------------
// Global state
// -----------------------------------------------------------------------------
float g_angle = 0.3f;   // Y-axis rotation (arrow keys)
GLuint g_VAO, g_VBO;
GLuint g_prog;

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
// drawBox: draws the unit cube with a given model matrix and solid colour.
//
// Parameters:
//   model  — the model matrix (scale + translate + rotate combined)
//   view   — camera view matrix (shared across all objects)
//   proj   — projection matrix (shared across all objects)
//   color  — RGB colour for this object
//
// To add a new object in Project 4, call this function with a new model matrix.
// -----------------------------------------------------------------------------
void drawBox(const glm::mat4& model,
    const glm::mat4& view,
    const glm::mat4& proj,
    glm::vec3 color)
{
    glm::mat4 mvp = proj * view * model;
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uMVP"),
        1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(g_prog, "uColor"),
        1, glm::value_ptr(color));
    glBindVertexArray(g_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// -----------------------------------------------------------------------------
// drawScene: builds and draws all objects in the scene.
//
// This is the function to expand in Project 4 — add more drawBox() calls
// here for each new object (chairs, monitors, plants, walls, etc.)
// -----------------------------------------------------------------------------
void drawScene(const glm::mat4& view, const glm::mat4& proj) {

    // Shared Y-axis rotation applied to the entire counter assembly
    // so the user can inspect it from different angles with arrow keys.
    glm::mat4 rot = glm::rotate(
        glm::mat4(1.0f),
        g_angle,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // COMPONENT 1: Front Panel (yellow facing)

    glm::mat4 panelModel = rot
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.55f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(2.4f, 1.1f, 0.4f));

    drawBox(panelModel, view, proj,
        glm::vec3(0.95f, 0.78f, 0.06f));   // yellow

    // Base panel - black block underneath yellow panel
    glm::mat4 baseModel = rot
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(2.4f, 0.2f, 0.5f));

    drawBox(baseModel, view, proj,
        glm::vec3(0.08f, 0.08f, 0.08f));   // black

    // Side panel — black block on left of yellow panel
    glm::mat4 sidePanelModel = rot
        * glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.3f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 1.0f, 0.5f));

    drawBox(sidePanelModel, view, proj,
        glm::vec3(0.08f, 0.08f, 0.08f));   // black

    // Side panel — black block on right of yellow panel
    glm::mat4 sidePanelModel1 = rot
        * glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 0.3f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 1.0f, 0.5f));

    drawBox(sidePanelModel1, view, proj,
        glm::vec3(0.08f, 0.08f, 0.08f));   // black

    // =========================================================================
    // PROJECT 4: Add more objects below this line following the same pattern:
    //
    // glm::mat4 myObjectModel = rot
    //     * glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z))
    //     * glm::scale(glm::mat4(1.0f), glm::vec3(w, h, d));
    // drawBox(myObjectModel, view, proj, glm::vec3(r, g, b));
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
    // ── GLFW init ─────────────────────────────────────────────────────────────
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

    // ── GLAD init ─────────────────────────────────────────────────────────────
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // ── Shaders ───────────────────────────────────────────────────────────────
    g_prog = createShaderProgram();

    // ── Upload unit cube to GPU ───────────────────────────────────────────────
    glGenVertexArrays(1, &g_VAO);
    glGenBuffers(1, &g_VBO);
    glBindVertexArray(g_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_cubeVerts), g_cubeVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Camera (view + projection) ────────────────────────────────────────────
    // For Project 4: adjust eye position and lookAt target to frame your scene.
    glm::mat4 view = glm::lookAt(
        glm::vec3(-2.0f, 1.2f, 4.5f),   // camera position
        glm::vec3(0.0f, 0.3f, 0.0f),   // look-at point
        glm::vec3(0.0f, 1.0f, 0.0f)    // world up
    );
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        (float)WIN_W / (float)WIN_H,
        0.1f,
        100.0f
    );

    // ── Render loop ───────────────────────────────────────────────────────────
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

    // ── Cleanup ───────────────────────────────────────────────────────────────
    glDeleteVertexArrays(1, &g_VAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteProgram(g_prog);
    glfwTerminate();
    return 0;
}