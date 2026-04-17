#include "triangulation-math.h"
#include "ov7670.h"
#include <math.h>

const float alpha_deg = 20.0f; // angle of the laser pointing inward (Degrees)
const float alpha_rad = alpha_deg * (PI / 180.0); 
const float f = 3.6f; // focal length of the camera lens in mm
const float p = 0.0072f; // physical size of a single pixel on the sensor in mm (Pixel Pitch). OV7670 QVGA: 0.0072 mm
const float x_center = OV7670_WIDTH / 2; // the middle column of the camera sensor (320 / 2 for QVGA)

const float b = 100.0f; // distance from camera to laser in millimeters
const float d = 274.7477419f; // distance to turntable center

float calculate_camera_depth(float brightest_pixel_x){
    return ((f * b) / ((brightest_pixel_x - x_center) * p + f * tan(alpha_rad)));
}

float calculate_theta(uint16_t step){
    return (step / 4096.0f) * 2.0 * PI;
}

float calculate_radial_depth(float camera_depth){
    return (d - camera_depth) / cos(alpha_rad);
}

p_pos_t calculate_xyz(float brightest_pixel_x, float row, uint16_t step){
    p_pos_t output;

    float camera_depth = calculate_camera_depth(brightest_pixel_x);
    float radial_depth = calculate_radial_depth(camera_depth);

    float theta = calculate_theta(step);

    output.x = radial_depth * cos(theta);
    output.y = -(row - (OV7670_HEIGHT / 2)) * p * (camera_depth / f);
    output.z = radial_depth * sin(theta);

    return output;
}