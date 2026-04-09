#include "opengl-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_WIDTH  320
#define FRAME_HEIGHT 240
#define FRAME_PIXELS (FRAME_WIDTH * FRAME_HEIGHT)
#define FRAME_RGB888_SIZE (FRAME_PIXELS * 3)

static GLuint shader_program;
static GLuint vertex_array_object;
static GLuint vertex_buffer_object;
static GLuint element_buffer_object;
static GLuint texture;

static uint8_t *rgb888_buffer;

static const char *vertex_source =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 1.0);\n"
    "    TexCoord = aTexCoord;\n"
    "}\n";

static const char *fragment_source =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D tex;\n"
    "void main() {\n"
    "    FragColor = texture(tex, TexCoord);\n"
    "}\n";

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        printf("Shader error: %s\n", info_log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint create_program(const char *vertex_src, const char *fragment_src) {
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_src);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_src);
    if (!vertex_shader || !fragment_shader) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        printf("Link error: %s\n", info_log);
        glDeleteProgram(program);
        program = 0;
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

static void rgb565_to_rgb888(const uint16_t *source, uint8_t *destination, int pixel_count) {
    for (int index = 0; index < pixel_count; index++) {
        uint16_t pixel = source[index];
        uint8_t red5   = (pixel >> 11) & 0x1F;
        uint8_t green6 = (pixel >> 5)  & 0x3F;
        uint8_t blue5  =  pixel        & 0x1F;
        destination[index * 3 + 0] = (red5 << 3)   | (red5 >> 2);
        destination[index * 3 + 1] = (green6 << 2)  | (green6 >> 4);
        destination[index * 3 + 2] = (blue5 << 3)   | (blue5 >> 2);
    }
}

void renderer_init(void) {
    shader_program = create_program(vertex_source, fragment_source);

    float vertices[] = {
        -0.5f,  -0.5f, 0.0f,  0.0f,  1.0f,
        0.5f,   -0.5f, 0.0f,  1.0f,  1.0f,
        0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
    };

    GLuint indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    glGenVertexArrays(1, &vertex_array_object);
    glGenBuffers(1, &vertex_buffer_object);
    glGenBuffers(1, &element_buffer_object);

    glBindVertexArray(vertex_array_object);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    uint8_t black[FRAME_RGB888_SIZE];
    memset(black, 0, sizeof(black));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, black);
    glBindTexture(GL_TEXTURE_2D, 0);

    rgb888_buffer = malloc(FRAME_RGB888_SIZE);
}

void renderer_upload_frame(const uint16_t *rgb565) {
    rgb565_to_rgb888(rgb565, rgb888_buffer, FRAME_PIXELS);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, rgb888_buffer);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderer_draw(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shader_program, "tex"), 0);
    glBindVertexArray(vertex_array_object);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderer_cleanup(void) {
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vertex_array_object);
    glDeleteBuffers(1, &vertex_buffer_object);
    glDeleteBuffers(1, &element_buffer_object);
    glDeleteProgram(shader_program);
    free(rgb888_buffer);
}
