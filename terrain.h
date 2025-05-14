#pragma once

#define GROUND_EPSILON 0.1

typedef struct
{
    Vector3 a,b,c;
    Vector3 normal;
    float height;
} Ground;

extern float terrain_scale_planar;
extern float terrain_scale_height;

void terrain_init();
void terrain_update();
void terrain_draw();

float terrain_get_ground(float x, float z, Ground* ground);
