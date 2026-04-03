cmake_minimum_required(VERSION 4.3.1)
project(3d_scan_viewer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME} src/main.c src/window.c src/opengl-functions.c)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL m)