cmake_minimum_required(VERSION 4.3.1)
project(3d_scan_viewer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.c
    src/window.c
    src/opengl-functions.c
    src/ov7670.c
    src/uart.c
    third_party/glad/src/glad.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)
target_include_directories(${PROJECT_NAME} PRIVATE third_party/glad/include)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL m ${CMAKE_DL_LIBS})