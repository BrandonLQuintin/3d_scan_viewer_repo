#include <glad/glad.h>
#include "opengl-functions.h"
#include "window.h"

int main(void) {
    if (initialize_window() != 0) {
        return 1;
    }

    renderer_init();

    while (!glfwWindowShouldClose(window)) {
        renderer_draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    renderer_cleanup();
    clean_window();
    return 0;
}