#include "ov7670.h"
#include "uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYNC_SIZE    4
#define RX_BUF_SIZE  4096

static const uint8_t sync_pattern[SYNC_SIZE] = {0xAD, 0xEE, 0xEE, 0xDE};

#define STATE_SYNC      0
#define STATE_DATA      1
#define STATE_BRIGHT    2

struct camera {
    int file_descriptor;
    uint8_t rx_buffer[RX_BUF_SIZE];
    uint8_t frame_buffer[OV7670_FRAME_BYTES];
    uint8_t brightest_buffer[OV7670_BRIGHTEST_BYTES];
    size_t data_offset;
    int sync_match;
    int state;
    int frame_ready;
};

camera_t *ov7670_open(const char *device, int baud) {
    camera_t *camera = calloc(1, sizeof(camera_t));
    if (!camera) return NULL;

    camera->file_descriptor = uart_open(device, baud);
    if (camera->file_descriptor < 0) {
        printf("Warning: could not open %s\n", device);
    }

    return camera;
}

int ov7670_get_fd(camera_t *camera){
    return camera->file_descriptor;
}

static void process_bytes(camera_t *camera, ssize_t byte_count) {
    size_t position = 0;
    while (position < (size_t)byte_count) {
        if (camera->state == STATE_SYNC) {
            while (position < (size_t)byte_count) {
                if (camera->rx_buffer[position] == sync_pattern[camera->sync_match]) {
                    camera->sync_match++;
                    if (camera->sync_match == SYNC_SIZE) {
                        camera->sync_match = 0;
                        camera->state = STATE_DATA;
                        camera->data_offset = 0;
                        position++;
                        break;
                    }
                } else {
                    camera->sync_match = (camera->rx_buffer[position] == sync_pattern[0]) ? 1 : 0;
                }
                position++;
            }
        }

        if (camera->state == STATE_DATA && position < (size_t)byte_count) {
            size_t remaining = byte_count - position;
            size_t needed = OV7670_FRAME_BYTES - camera->data_offset;
            size_t to_copy = remaining < needed ? remaining : needed;
            memcpy(camera->frame_buffer + camera->data_offset, camera->rx_buffer + position, to_copy);
            camera->data_offset += to_copy;
            position += to_copy;

            if (camera->data_offset >= OV7670_FRAME_BYTES) {
                camera->state = STATE_BRIGHT;
                camera->data_offset = 0;
            }
        }

        if (camera->state == STATE_BRIGHT && position < (size_t)byte_count) {
            size_t remaining = byte_count - position;
            size_t needed = OV7670_BRIGHTEST_BYTES - camera->data_offset;
            size_t to_copy = remaining < needed ? remaining : needed;
            memcpy(camera->brightest_buffer + camera->data_offset, camera->rx_buffer + position, to_copy);
            camera->data_offset += to_copy;
            position += to_copy;

            if (camera->data_offset >= OV7670_BRIGHTEST_BYTES) {
                camera->frame_ready = 1;
                camera->state = STATE_SYNC;
                camera->sync_match = 0;
                camera->data_offset = 0;
            }
        }
    }
}

int ov7670_read_frame(camera_t *camera, const uint16_t **out_frame, const uint16_t **out_brightest) {
    if (camera->file_descriptor < 0) return 0;

    ssize_t bytes_read = uart_read(camera->file_descriptor, camera->rx_buffer, RX_BUF_SIZE);
    if (bytes_read <= 0) return 0;

    process_bytes(camera, bytes_read);

    if (camera->frame_ready) {
        camera->frame_ready = 0;
        *out_frame = (const uint16_t *)camera->frame_buffer;
        *out_brightest = (const uint16_t *)camera->brightest_buffer;
        return 1;
    }

    return 0;
}

void ov7670_close(camera_t *camera) {
    if (!camera) return;
    uart_close(camera->file_descriptor);
    free(camera);
}
