#include <GL/gl.h>
#include "opengl-functions.h"
#include "window.h"


int main(void){
    if (initialize_window() == 1){
        return 1;
    }

    while (!glfwWindowShouldClose(window)){
        renderTriangle();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    clean_window();

    return 0;
}