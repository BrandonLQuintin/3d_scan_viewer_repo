#include <glad/glad.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ov7670.h"
#include "triangulation-math.h"
#include "uart.h"
#include "window.h"
#if !DISABLE_2D_RENDERER
#include "renderer-image.h"
#else
#include "renderer-3d.h"
#endif

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

#if !DISABLE_2D_RENDERER
    renderer_init();
#else
    renderer_3d_init();
#endif

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
            p_pos_t *temp = realloc(final_xyz_positions, (OV7670_HEIGHT + xyz_ptr_ctr) * sizeof(p_pos_t));
            if (!temp) {
                return_value = 1;
                goto cleanup;
            } else {
                final_xyz_positions = temp;
            }

#if !DISABLE_2D_RENDERER
            if (frame) {
                renderer_upload_frame(frame, brightest);
            }
#endif
            for (int i = 0; i < OV7670_HEIGHT; i++){
                p_pos_t pt = calculate_xyz(brightest[i], (float)i, ov7670_get_step(camera));
                final_xyz_positions[xyz_ptr_ctr] = pt;
                xyz_ptr_ctr += 1;
            }
            uint8_t ack_value = 23;
            uart_write(ov7670_get_fd(camera), &ack_value, 1);

        }

#if !DISABLE_2D_RENDERER
        renderer_draw();
#else
        renderer_3d_upload_points(final_xyz_positions, xyz_ptr_ctr);
        renderer_3d_draw();
#endif
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
cleanup:
    free(final_xyz_positions);
    ov7670_close(camera);
#if !DISABLE_2D_RENDERER
    renderer_cleanup();
#else
    renderer_3d_cleanup();
#endif
    clean_window();
    return return_value;
}
