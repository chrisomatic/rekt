#pragma once

#include "terrain.h"

typedef enum
{
    VIEWPOINT_FIRST,
    VIEWPOINT_THIRD,
} ViewPoint;

typedef struct
{
    Vector3 vel;
    Vector3 pos;
    Vector3 target;
    float height;
    float run_speed;
    float jump_speed;
    float angle_theta; // horizontal. degrees
    float angle_omega; // vertical. degrees
    ViewPoint viewpoint;
    Ground ground;
    bool running;
} Player;

extern Player player;
extern Camera camera;

void player_init();
void player_update(float dt);
void player_draw();
