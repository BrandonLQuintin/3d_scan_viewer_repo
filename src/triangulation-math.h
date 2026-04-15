#ifndef TRIANGULATION_MATH_H_
#define TRIANGULATION_MATH_H_

#include <stdint.h>

#define PI 3.14159265359f

typedef struct {
    float x;
    float y;
    float z;
} pixel_position;
typedef pixel_position p_pos_t;

float calculate_camera_depth(float brightest_pixel_x);
float calculate_radial_depth(float camera_depth);
float calculate_theta(uint16_t step);
p_pos_t calculate_xyz(float brightest_pixel_x, float row, uint16_t step);

#endif