#include "renderer-3d.h"
#include "window.h"
#include <glad/glad.h>
#include <linmath.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint shader_program;
static GLuint vao;
static GLuint vbo;
static size_t point_count;

static float cam_dist = 150.0f;
static float cam_rot_x = 0.4f;
static float cam_rot_y = 0.0f;
static double last_mouse_x;
static double last_mouse_y;
static int mouse_dragging;

static const char *vertex_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform float point_size;\n"
    "void main() {\n"
    "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "    gl_PointSize = point_size / max(gl_Position.w, 0.001);\n"
    "}\n";

static const char *fragment_src =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec3 lightDir;\n"
    "void main() {\n"
    "    vec2 coord = gl_PointCoord - vec2(0.5);\n"
    "    float dist2 = dot(coord, coord);\n"
    "    if (dist2 > 0.25) discard;\n"
    "    float z = sqrt(0.25 - dist2);\n"
    "    vec3 normal = normalize(vec3(coord, z));\n"
    "    vec3 ld = normalize(lightDir);\n"
    "    float ambient = 0.15;\n"
    "    float diffuse = max(dot(normal, ld), 0.0) * 0.7;\n"
    "    vec3 viewDir = vec3(0.0, 0.0, 1.0);\n"
    "    vec3 refl = reflect(-ld, normal);\n"
    "    float spec = pow(max(dot(viewDir, refl), 0.0), 32.0) * 0.4;\n"
    "    vec3 color = vec3(0.3, 0.6, 0.9) * (ambient + diffuse) + vec3(1.0) * spec;\n"
    "    FragColor = vec4(color, 1.0);\n"
    "}\n";

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        printf("Shader error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint create_program(const char *vs, const char *fs) {
    GLuint v = compile_shader(GL_VERTEX_SHADER, vs);
    GLuint f = compile_shader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) {
        glDeleteShader(v);
        glDeleteShader(f);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, NULL, log);
        printf("Link error: %s\n", log);
        glDeleteProgram(prog);
        prog = 0;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return prog;
}

static void mouse_button_callback(GLFWwindow *w, int button, int action, int mods) {
    (void)w; (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mouse_dragging = (action == GLFW_PRESS);
        if (mouse_dragging) {
            double xpos, ypos;
            glfwGetCursorPos(w, &xpos, &ypos);
            last_mouse_x = xpos;
            last_mouse_y = ypos;
        }
    }
}

static void cursor_callback(GLFWwindow *w, double xpos, double ypos) {
    (void)w;
    if (!mouse_dragging) return;
    float dx = (float)(xpos - last_mouse_x) * 0.01f;
    float dy = (float)(ypos - last_mouse_y) * 0.01f;
    cam_rot_y += dx;
    cam_rot_x += dy;
    if (cam_rot_x > 1.5f) cam_rot_x = 1.5f;
    if (cam_rot_x < -1.5f) cam_rot_x = -1.5f;
    last_mouse_x = xpos;
    last_mouse_y = ypos;
}

static void scroll_callback(GLFWwindow *w, double xoffset, double yoffset) {
    (void)w; (void)xoffset;
    cam_dist -= (float)yoffset * 20.0f;
    if (cam_dist < 10.0f) cam_dist = 10.0f;
    if (cam_dist > 2000.0f) cam_dist = 2000.0f;
}

void renderer_3d_init(void) {
    shader_program = create_program(vertex_src, fragment_src);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(p_pos_t), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
}

void renderer_3d_upload_points(const p_pos_t *points, size_t count) {
    point_count = count;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    size_t size = count * sizeof(p_pos_t);
    glBufferData(GL_ARRAY_BUFFER, size, points, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void renderer_3d_draw(void) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

    mat4x4 proj;
    mat4x4_perspective(proj, 1.0f, aspect, 1.0f, 5000.0f);

    vec3 eye;
    eye[0] = cam_dist * cosf(cam_rot_x) * sinf(cam_rot_y);
    eye[1] = cam_dist * sinf(cam_rot_x);
    eye[2] = cam_dist * cosf(cam_rot_x) * cosf(cam_rot_y);
    vec3 center = {0, 0, 0};
    vec3 up = {0, 1, 0};

    mat4x4 view;
    mat4x4_look_at(view, eye, center, up);

    mat4x4 model;
    mat4x4_identity(model);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_program);

    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, (const float *)proj);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, (const float *)view);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, (const float *)model);

    glUniform1f(glGetUniformLocation(shader_program, "point_size"), 800.0f);
    glUniform3f(glGetUniformLocation(shader_program, "lightDir"), 0.5f, 0.8f, 0.3f);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, (GLsizei)point_count);
    glBindVertexArray(0);
}

void renderer_3d_cleanup(void) {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}
