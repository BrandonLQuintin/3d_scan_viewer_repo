#ifndef OV7670_H
#define OV7670_H

#include <stdint.h>
#include <stddef.h>
#include <termios.h>

#define DISABLE_FRAME_BUFFER 1
#define DISABLE_2D_RENDERER 1

#define OV7670_WIDTH  320
#define OV7670_HEIGHT 240
#define OV7670_PIXELS (OV7670_WIDTH * OV7670_HEIGHT)
#define OV7670_FRAME_BYTES (OV7670_PIXELS * 2)
#define OV7670_ROW_COUNT OV7670_HEIGHT
#define OV7670_BRIGHTEST_BYTES (OV7670_ROW_COUNT * 2)
#define RADIAL_FILTER_MM 75.0f

typedef struct camera camera_t;

camera_t *ov7670_open(const char *device, int baud);
int ov7670_get_fd(camera_t *camera);
uint16_t ov7670_get_step(camera_t *camera);
int ov7670_read_frame(camera_t *camera, const uint16_t **out_frame, const uint16_t **out_brightest);
void ov7670_close(camera_t *camera);

#endif
