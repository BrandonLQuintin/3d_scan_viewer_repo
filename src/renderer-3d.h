#ifndef RENDERER_3D_H
#define RENDERER_3D_H

#include "triangulation-math.h"
#include <stddef.h>

void renderer_3d_init(void);
void renderer_3d_upload_points(const p_pos_t *points, size_t count);
void renderer_3d_draw(void);
void renderer_3d_cleanup(void);

#endif
