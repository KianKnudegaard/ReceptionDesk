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
const char* TITLE = "Front Desk Lobby Scene  [WASD + Arrows to navigate]";

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

const float CUBE_VERTS[] = {
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
     0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
     0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
};

GLuint g_cubeVAO, g_cubeVBO;
GLuint g_cylVAO, g_cylVBO;
GLuint g_prog;
int    g_cylVertCount = 0;

glm::vec3 g_camPos = glm::vec3(-2.0f, 1.6f, 5.0f);
glm::vec3 g_camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 g_camUp = glm::vec3(0.0f, 1.0f, 0.0f);
float     g_yaw = -75.0f;   // angled slightly right toward counter
float     g_pitch = -8.0f;

void buildCylinder(int segments = 24) {
    std::vector<float> verts;
    float r = 0.5f;
    float pi = 3.14159265f;
    for (int i = 0; i < segments; ++i) {
        float a0 = 2 * pi * i / segments, a1 = 2 * pi * (i + 1) / segments;
        float x0 = r * cosf(a0), z0 = r * sinf(a0);
        float x1 = r * cosf(a1), z1 = r * sinf(a1);
        verts.insert(verts.end(), { x0,-0.5f,z0, x1,-0.5f,z1, x1,0.5f,z1 });
        verts.insert(verts.end(), { x0,-0.5f,z0, x1,0.5f,z1,  x0,0.5f,z0 });
        verts.insert(verts.end(), { 0,0.5f,0, x0,0.5f,z0, x1,0.5f,z1 });
        verts.insert(verts.end(), { 0,-0.5f,0, x1,-0.5f,z1, x0,-0.5f,z0 });
    }
    g_cylVertCount = (int)verts.size() / 3;
    glGenVertexArrays(1, &g_cylVAO);
    glGenBuffers(1, &g_cylVBO);
    glBindVertexArray(g_cylVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_cylVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
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

void drawBox(const glm::mat4& model, const glm::mat4& view,
    const glm::mat4& proj, glm::vec3 color) {
    glm::mat4 mvp = proj * view * model;
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(g_prog, "uColor"), 1, glm::value_ptr(color));
    glBindVertexArray(g_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}
void drawCylinder(const glm::mat4& model, const glm::mat4& view,
    const glm::mat4& proj, glm::vec3 color) {
    glm::mat4 mvp = proj * view * model;
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(g_prog, "uColor"), 1, glm::value_ptr(color));
    glBindVertexArray(g_cylVAO);
    glDrawArrays(GL_TRIANGLES, 0, g_cylVertCount);
}

const glm::vec3 COL_FLOOR = glm::vec3(0.72f, 0.70f, 0.68f);
const glm::vec3 COL_CEILING = glm::vec3(0.90f, 0.90f, 0.88f);
const glm::vec3 COL_WALL = glm::vec3(0.88f, 0.87f, 0.84f);
const glm::vec3 COL_ACCENT_WALL = glm::vec3(0.25f, 0.15f, 0.10f);
const glm::vec3 COL_COUNTER_FACE = glm::vec3(0.95f, 0.78f, 0.06f);
const glm::vec3 COL_COUNTER_TOP = glm::vec3(0.88f, 0.88f, 0.86f);
const glm::vec3 COL_BLACK = glm::vec3(0.08f, 0.08f, 0.08f);
const glm::vec3 COL_DESK = glm::vec3(0.22f, 0.16f, 0.12f);
const glm::vec3 COL_TV_BEZEL = glm::vec3(0.08f, 0.08f, 0.08f);
const glm::vec3 COL_TV_SCREEN = glm::vec3(0.20f, 0.35f, 0.70f);
const glm::vec3 COL_WHITEBOARD = glm::vec3(0.95f, 0.95f, 0.95f);
const glm::vec3 COL_PENDANT = glm::vec3(0.12f, 0.12f, 0.12f);
const glm::vec3 COL_CHAIR = glm::vec3(0.08f, 0.08f, 0.08f);
const glm::vec3 COL_DOOR_FRAME = glm::vec3(0.10f, 0.10f, 0.10f);
const glm::vec3 COL_DOOR_GLASS = glm::vec3(0.50f, 0.65f, 0.75f);

glm::mat4 makeModel(glm::vec3 scale, glm::vec3 translate, float rotYDeg = 0.0f) {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, translate);
    if (rotYDeg != 0.0f)
        m = glm::rotate(m, glm::radians(rotYDeg), glm::vec3(0, 1, 0));
    m = glm::scale(m, scale);
    return m;
}

void drawChair(float cx, float cz, const glm::mat4& view, const glm::mat4& proj) {
    drawBox(makeModel({ 0.5f,0.06f,0.5f }, { cx,0.55f,cz }), view, proj, COL_CHAIR);
    drawBox(makeModel({ 0.48f,0.5f,0.06f }, { cx,0.85f,cz - 0.22f }), view, proj, COL_CHAIR);
    drawCylinder(makeModel({ 0.06f,0.55f,0.06f }, { cx,0.27f,cz }), view, proj, COL_CHAIR);
    drawCylinder(makeModel({ 0.45f,0.04f,0.45f }, { cx,0.04f,cz }), view, proj, COL_CHAIR);
}

void drawPendantLight(float px, float pz, const glm::mat4& view, const glm::mat4& proj) {
    drawCylinder(makeModel({ 0.02f,0.8f,0.02f }, { px,2.8f,pz }), view, proj, COL_PENDANT);
    drawCylinder(makeModel({ 0.12f,0.4f,0.12f }, { px,2.38f,pz }), view, proj, COL_PENDANT);
}

void drawScene(const glm::mat4& view, const glm::mat4& proj) {

    // Counter X offset — all counter pieces share this anchor
    const float CX = 1.2f;
    // Counter Z position (front face of yellow panel)
    const float CZ = -1.0f;

    // FLOOR
    drawBox(makeModel({ 8.0f,0.05f,7.0f }, { 0.0f,-0.025f,0.0f }),
        view, proj, COL_FLOOR);

    // CEILING
    drawBox(makeModel({ 8.0f,0.1f,7.0f }, { 0.0f,3.2f,0.0f }),
        view, proj, COL_CEILING);

    // BACK WALL
    drawBox(makeModel({ 8.0f,3.2f,0.1f }, { 0.0f,1.55f,-3.5f }),
        view, proj, COL_WALL);

    // RECEPTION COUNTER YELLOW FRONT PANEL
    drawBox(makeModel({ 2.4f,1.2f,0.3f }, { CX,0.55f,CZ }), 
        view, proj, COL_COUNTER_FACE);

    // COUNTER BASE 
    drawBox(makeModel({ 2.4f,0.6f,0.5f }, { CX,-0.1f,CZ }),
        view, proj, COL_BLACK);

    // COUNTER SIDE PANELS
    drawBox(makeModel({ 0.2f,1.2f,0.5f }, { CX - 1.3f,0.3f,CZ }),
        view, proj, COL_BLACK);
    drawBox(makeModel({ 0.2f,1.2f,0.5f }, { CX + 1.3f,0.3f,CZ }),
        view, proj, COL_BLACK);

    // WORK DESK (dark wood, left of counter)
    drawBox(makeModel({ 1.6f,0.06f,0.9f }, { -1.0f,0.76f,-1.0f }), 
        view, proj, COL_DESK);
    drawBox(makeModel({ 0.1f,0.8f,0.85f }, { -0.3f,0.38f,-1.0f }),
        view, proj, COL_DESK);
    drawBox(makeModel({ 0.1f,0.8f,0.85f }, { -1.7f,0.38f,-1.0f }),
        view, proj, COL_DESK);

    // WALL-MOUNTED TV
    drawBox(makeModel({ 1.2f,0.72f,0.06f }, { 1.0f,1.9f,-3.44f }),
        view, proj, COL_TV_BEZEL);
    drawBox(makeModel({ 1.1f,0.62f,0.04f }, { 1.0f,1.9f,-3.40f }),
        view, proj, COL_TV_SCREEN);

    // WHITEBOARD
    drawBox(makeModel({ 0.7f,1.2f,0.04f }, { 2.2f,1.7f,-3.4f }),
        view, proj, COL_WHITEBOARD);

    // PENDANT LIGHTS x2
    drawPendantLight(CX - 0.9f, CZ - 0.8f, view, proj);
    drawPendantLight(CX + 0.3f, CZ - 0.8f, view, proj);

    // OFFICE CHAIRS x2
    drawChair(-0.73f, -1.4f, view, proj);
    drawChair(-1.3f, -1.4f, view, proj);

    // ENTRANCE DOOR (right side of scene)
    drawBox(makeModel({ 2.0f,2.6f,0.06f }, { 4.0f,1.3f,0.5f}, 90.0f),
        view, proj, COL_DOOR_FRAME);
    drawBox(makeModel({ 0.8f,2.55f,0.1f }, { 4.0f,1.3f,0.0f}, 90.0f),
        view, proj, COL_DOOR_GLASS);
    drawBox(makeModel({ 0.8f,2.55f,0.1f }, { 4.0f,1.3f,1.0f}, 90.0f),
        view, proj, COL_DOOR_GLASS);
}

void framebufferSizeCallback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }
void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void updateCamera() {
    glm::vec3 front;
    front.x = cosf(glm::radians(g_yaw)) * cosf(glm::radians(g_pitch));
    front.y = sinf(glm::radians(g_pitch));
    front.z = sinf(glm::radians(g_yaw)) * cosf(glm::radians(g_pitch));
    g_camFront = glm::normalize(front);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, TITLE, nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    g_prog = createProgram();

    glGenVertexArrays(1, &g_cubeVAO);
    glGenBuffers(1, &g_cubeVBO);
    glBindVertexArray(g_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    buildCylinder(24);

    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        (float)WIN_W / (float)WIN_H,
        0.1f, 100.0f
    );

    float moveSpeed = 2.5f;
    float turnSpeed = 60.0f;
    double lastTime = glfwGetTime();

    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
    std::cout << "WASD = move  |  Arrow keys = look  |  ESC = quit\n";

    // Initialise camera direction from starting yaw/pitch
    updateCamera();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float  dt = (float)(now - lastTime);
        lastTime = now;

        glfwPollEvents();

        float ms = moveSpeed * dt;
        float ts = turnSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) g_camPos += ms * g_camFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) g_camPos -= ms * g_camFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            g_camPos -= glm::normalize(glm::cross(g_camFront, g_camUp)) * ms;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            g_camPos += glm::normalize(glm::cross(g_camFront, g_camUp)) * ms;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) g_yaw -= ts;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_yaw += ts;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) g_pitch += ts;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) g_pitch -= ts;
        g_pitch = glm::clamp(g_pitch, -89.0f, 89.0f);
        updateCamera();

        glm::mat4 view = glm::lookAt(g_camPos, g_camPos + g_camFront, g_camUp);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_prog);
        drawScene(view, proj);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &g_cubeVAO);
    glDeleteBuffers(1, &g_cubeVBO);
    glDeleteVertexArrays(1, &g_cylVAO);
    glDeleteBuffers(1, &g_cylVBO);
    glDeleteProgram(g_prog);
    glfwTerminate();
    return 0;
}