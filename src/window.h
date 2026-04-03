#ifndef WINDOW_H
#include <GLFW/glfw3.h>

extern GLFWwindow *window;

int initialize_window();
void clean_window();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

#endif