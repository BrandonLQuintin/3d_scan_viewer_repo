#include "window.h"
#include <GL/gl.h>

GLFWwindow *window;

int initialize_window(){
    if (!glfwInit()){
        return 1;
    }

    window = glfwCreateWindow(800, 600, "3D Scan Viewer", NULL, NULL);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!window){
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    return 0;
}

void clean_window(){
    glfwDestroyWindow(window);
    glfwTerminate();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}