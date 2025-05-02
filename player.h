#pragma once

typedef struct
{
    Vector3 vel;
    Vector3 pos;
    float speed;
    float height;
} Player;

extern Player player;
extern Camera camera;

void player_init();
void player_update(float dt);
void player_draw();
