#ifndef OPENGL_FUNCTIONS_H
#define OPENGL_FUNCTIONS_H

#include <glad/glad.h>
#include <stdint.h>

void renderer_init(void);
void renderer_upload_frame(const uint16_t *rgb565, const uint16_t *brightest_x);
void renderer_draw(void);
void renderer_cleanup(void);

#endif