#pragma once

typedef struct
{
    Vector3 vel;
    Vector3 pos;
    Vector3 pos_prior;
    Vector3 target;
    float height;
    float run_speed;
    float jump_speed;
    float facing_angle; // degrees
} Player;

extern Player player;
extern Camera camera;

void player_init();
void player_update(float dt);
void player_draw();
