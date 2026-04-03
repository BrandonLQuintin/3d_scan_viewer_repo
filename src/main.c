#include <GL/gl.h>
#include <GLFW/glfw3.h>

int main(void){
    if (!glfwInit()){
        return 1;
    }

    GLFWwindow *window = glfwCreateWindow(800, 600, "3D Scan Viewer", NULL, NULL);
    if (!window){
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)){
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
        glColor3f(0.0f,1.0f, 0.0f); glVertex2f(0.5f, -0.5f);
        glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(0.0f, 0.5f);
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}