#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

const int WIN_W = 900;
const int WIN_H = 700;

const int   GRID = 60;
const float RANGE = 6.0f;  

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

float g_rotY = 0.3f;
float g_rotX = 0.4f;

GLuint g_VAO, g_VBO, g_EBO, g_prog;
int    g_indexCount;

float f(float x, float z) {
    return sinf(x) * cosf(z);
}

glm::vec3 colorForHeight(float y) {
    float t = (y + 1.0f) * 0.5f;  
    if (t < 0.5f) {
        // blue -> green
        return glm::mix(glm::vec3(0.1f, 0.3f, 1.0f),
            glm::vec3(0.1f, 0.9f, 0.3f), t * 2.0f);
    }
    else {
        // green -> red
        return glm::mix(glm::vec3(0.1f, 0.9f, 0.3f),
            glm::vec3(1.0f, 0.2f, 0.1f), (t - 0.5f) * 2.0f);
    }
}

void buildMesh() {
    std::vector<float>        verts;
    std::vector<unsigned int> indices;

    float step = (2.0f * RANGE) / GRID;

    for (int iz = 0; iz <= GRID; ++iz) {
        for (int ix = 0; ix <= GRID; ++ix) {
            float x = -RANGE + ix * step;
            float z = -RANGE + iz * step;
            float y = f(x, z);

            glm::vec3 col = colorForHeight(y);

            verts.push_back(x);
            verts.push_back(y);
            verts.push_back(z);
            verts.push_back(col.r);
            verts.push_back(col.g);
            verts.push_back(col.b);
        }
    }

    for (int iz = 0; iz <= GRID; ++iz) {
        for (int ix = 0; ix < GRID; ++ix) {
            int a = iz * (GRID + 1) + ix;
            indices.push_back(a);
            indices.push_back(a + 1);
        }
    }
    for (int ix = 0; ix <= GRID; ++ix) {
        for (int iz = 0; iz < GRID; ++iz) {
            int a = iz * (GRID + 1) + ix;
            indices.push_back(a);
            indices.push_back(a + (GRID + 1));
        }
    }

    g_indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &g_VAO);
    glGenBuffers(1, &g_VBO);
    glGenBuffers(1, &g_EBO);

    glBindVertexArray(g_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
        verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

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

void framebufferSizeCallback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }
void keyCallback(GLFWwindow* w, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H,
        "Mesh: f(x,z) = sin(x)*cos(z)  [Arrow keys to rotate]",
        nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    g_prog = createProgram();
    buildMesh();

    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        (float)WIN_W / (float)WIN_H,
        0.1f, 100.0f
    );

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 8.0f, 14.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    std::cout << "Arrow keys to rotate. ESC to quit.\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) g_rotY -= 0.02f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_rotY += 0.02f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) g_rotX -= 0.02f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) g_rotX += 0.02f;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_prog);

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), g_rotY, glm::vec3(0, 1, 0));
        model = glm::rotate(model, g_rotX, glm::vec3(1, 0, 0));

        glm::mat4 mvp = proj * view * model;
        glUniformMatrix4fv(glGetUniformLocation(g_prog, "uMVP"),
            1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(g_VAO);
        glDrawElements(GL_LINES, g_indexCount, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &g_VAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteBuffers(1, &g_EBO);
    glDeleteProgram(g_prog);
    glfwTerminate();
    return 0;
}