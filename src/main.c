#include <glad/glad.h>
#include "opengl-functions.h"
#include "ov7670.h"
#include "window.h"

#define UART_DEVICE "/dev/ttyACM0"
#define UART_BAUD B115200

int main(void) {
    if (initialize_window() != 0) {
        return 1;
    }

    renderer_init();

    camera_t *camera = ov7670_open(UART_DEVICE, UART_BAUD);

    while (!glfwWindowShouldClose(window)) {
        const uint16_t *frame;
        if (ov7670_read_frame(camera, &frame)) {
            renderer_upload_frame(frame);
        }

        renderer_draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ov7670_close(camera);
    renderer_cleanup();
    clean_window();
    return 0;
}