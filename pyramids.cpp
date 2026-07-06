#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

const int WIN_W = 900;
const int WIN_H = 700;

const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* FRAG_SRC = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

float pyramidVerts[] = {
    // Front face        — orange
     0.0f, 1.0f, 0.0f,   1.0f, 0.55f, 0.0f,
    -0.5f, 0.0f, 0.5f,   1.0f, 0.55f, 0.0f,
     0.5f, 0.0f, 0.5f,   1.0f, 0.55f, 0.0f,

     // Right face        — red
      0.0f, 1.0f, 0.0f,   0.8f, 0.1f,  0.1f,
      0.5f, 0.0f, 0.5f,   0.8f, 0.1f,  0.1f,
      0.5f, 0.0f,-0.5f,   0.8f, 0.1f,  0.1f,

      // Back face         — dark orange
       0.0f, 1.0f, 0.0f,   0.7f, 0.35f, 0.0f,
       0.5f, 0.0f,-0.5f,   0.7f, 0.35f, 0.0f,
      -0.5f, 0.0f,-0.5f,   0.7f, 0.35f, 0.0f,

      // Left face         — yellow
       0.0f, 1.0f, 0.0f,   0.9f, 0.8f,  0.1f,
      -0.5f, 0.0f,-0.5f,   0.9f, 0.8f,  0.1f,
      -0.5f, 0.0f, 0.5f,   0.9f, 0.8f,  0.1f,

      // Base triangle 1   — brown
      -0.5f, 0.0f, 0.5f,   0.4f, 0.25f, 0.1f,
       0.5f, 0.0f, 0.5f,   0.4f, 0.25f, 0.1f,
       0.5f, 0.0f,-0.5f,   0.4f, 0.25f, 0.1f,

       // Base triangle 2   — brown
       -0.5f, 0.0f, 0.5f,   0.4f, 0.25f, 0.1f,
        0.5f, 0.0f,-0.5f,   0.4f, 0.25f, 0.1f,
       -0.5f, 0.0f,-0.5f,   0.4f, 0.25f, 0.1f,
};

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); std::cerr << log; }
    return s;
}
GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

GLuint g_VAO, g_VBO, g_prog;
int    g_camera = 1;   // active camera (1, 2, or 3)

struct Camera { glm::vec3 eye, target, up; const char* label; };

Camera g_cameras[3] = {
    { glm::vec3(0.0f, 1.0f,  5.0f),
      glm::vec3(0.0f, 0.5f,  0.0f),
      glm::vec3(0.0f, 1.0f,  0.0f),
      "Camera 1: Front View" },

      { glm::vec3(5.0f, 4.0f,  3.0f),
        glm::vec3(0.0f, 0.5f,  0.0f),
        glm::vec3(0.0f, 1.0f,  0.0f),
        "Camera 2: Elevated Side Angle" },

        { glm::vec3(0.0f, 7.0f,  0.1f),
          glm::vec3(0.0f, 0.0f,  0.0f),
          glm::vec3(0.0f, 1.0f,  0.0f),
          "Camera 3: Bird's Eye View" },
};

void drawPyramid(float xOffset, const glm::mat4& view, const glm::mat4& proj) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(xOffset, 0.0f, 0.0f));
    glm::mat4 mvp = proj * view * model;
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glBindVertexArray(g_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 18);
}

void framebufferSizeCallback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_1) { g_camera = 1; std::cout << g_cameras[0].label << "\n"; }
    if (key == GLFW_KEY_2) { g_camera = 2; std::cout << g_cameras[1].label << "\n"; }
    if (key == GLFW_KEY_3) { g_camera = 3; std::cout << g_cameras[2].label << "\n"; }
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H,
        "Three Pyramids — Press 1/2/3 to switch cameras", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    g_prog = createProgram();

    // Upload pyramid geometry
    glGenVertexArrays(1, &g_VAO);
    glGenBuffers(1, &g_VBO);
    glBindVertexArray(g_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVerts), pyramidVerts, GL_STATIC_DRAW);
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        (float)WIN_W / (float)WIN_H,
        0.1f, 100.0f
    );

    std::cout << "Press 1, 2, or 3 to switch camera views. ESC to quit.\n";
    std::cout << g_cameras[0].label << "\n";

    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);   // grey background

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_prog);

        Camera& cam = g_cameras[g_camera - 1];
        glm::mat4 view = glm::lookAt(cam.eye, cam.target, cam.up);

        drawPyramid(-2.0f, view, proj);   // left
        drawPyramid(0.0f, view, proj);   // centre
        drawPyramid(2.0f, view, proj);   // right

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &g_VAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteProgram(g_prog);
    glfwTerminate();
    return 0;
}