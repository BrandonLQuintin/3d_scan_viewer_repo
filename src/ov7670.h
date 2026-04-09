#ifndef OV7670_H
#define OV7670_H

#include <stdint.h>
#include <stddef.h>
#include <termios.h>

#define OV7670_WIDTH  320
#define OV7670_HEIGHT 240
#define OV7670_PIXELS (OV7670_WIDTH * OV7670_HEIGHT)
#define OV7670_FRAME_BYTES (OV7670_PIXELS * 2)

typedef struct camera camera_t;

camera_t *ov7670_open(const char *device, int baud);
int ov7670_read_frame(camera_t *cam, const uint16_t **out_frame);
void ov7670_close(camera_t *cam);

#endif
