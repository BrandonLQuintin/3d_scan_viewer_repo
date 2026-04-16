#include <glad/glad.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "opengl-functions.h"
#include "ov7670.h"
#include "triangulation-math.h"
#include "uart.h"
#include "window.h"

#define UART_DEVICE "/dev/ttyACM0"
#define UART_BAUD B115200

int main(void) {
    uint8_t return_value = 0;
    camera_t *camera = NULL;
    p_pos_t *final_xyz_positions = NULL;
    size_t xyz_ptr_ctr = 0;

    if (initialize_window() != 0) {
        return_value = 1;
        goto cleanup;
    }

    renderer_init();

    camera = ov7670_open(UART_DEVICE, UART_BAUD);
    if (!camera){
        return_value = 1;
        goto cleanup;
    }
    final_xyz_positions = malloc(OV7670_HEIGHT * sizeof(p_pos_t));
    if (!final_xyz_positions){
        return_value = 1;
        goto cleanup;
    }

    while (!glfwWindowShouldClose(window)) {
        const uint16_t *frame;
        const uint16_t *brightest;
        if (ov7670_read_frame(camera, &frame, &brightest)) {
            // TODO: Add a hard limit of points for this allocation once I figure out what the limit is.
            p_pos_t *temp = realloc(final_xyz_positions, (OV7670_HEIGHT + xyz_ptr_ctr) * sizeof(p_pos_t));
            if (!temp) {
                return_value = 1;
                goto cleanup;
            } else {
                final_xyz_positions = temp;
            }

            if (frame) {
                renderer_upload_frame(frame, brightest);
            }
            for (int i = 0; i < OV7670_HEIGHT; i++){
                final_xyz_positions[xyz_ptr_ctr] = calculate_xyz(brightest[i], (float)i, ov7670_get_step(camera));
                printf("brightest[%d]: %d\n", i, brightest[i]);
                printf("pixel[%d] x: %f, y: %f, z: %f\n", (int)xyz_ptr_ctr, final_xyz_positions[xyz_ptr_ctr].x, final_xyz_positions[xyz_ptr_ctr].y, final_xyz_positions[xyz_ptr_ctr].z);
                xyz_ptr_ctr += 1;
            }
            uint8_t ack_value = 23;
            uart_write(ov7670_get_fd(camera), &ack_value, 1);

        }

        renderer_draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
cleanup:
    free(final_xyz_positions);
    ov7670_close(camera);
    renderer_cleanup();
    clean_window();
    return return_value;
}