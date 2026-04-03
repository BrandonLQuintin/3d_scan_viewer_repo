#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>

extern GLFWwindow *window;

int initialize_window();
void clean_window();

#endif