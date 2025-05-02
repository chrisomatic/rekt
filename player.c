#include <stdio.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "common.h"
#include "player.h"

#define CAMERA_ROTATION_SPEED                           0.03f
#define CAMERA_PAN_SPEED                                0.2f
#define CAMERA_MOUSE_MOVE_SENSITIVITY                   0.003f

Player player = {0};
Camera camera = {0};
Model girl;

void player_init()
{
    player.pos = (Vector3){ 0.0f, 0.0f, 0.0f }; // @ feet
    player.speed = 5.4;
    player.height = 1.6;

    camera.position = (Vector3){ 0.0f, player.height*0.90, 5.0f }; // set camera at eye-level
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    girl = LoadModel("models/female1.obj");
    Texture2D texture = LoadTexture("models/female1.png");
    girl.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
}

void player_update(float dt)
{
    player.vel = Vector3Zero();

    Vector3 rot = Vector3Zero();
    Vector3 mov = Vector3Zero();

    if(IsKeyDown(KEY_W)) { mov.x = +1.0; rot.y = +1.0; }
    if(IsKeyDown(KEY_S)) { mov.x = -1.0; rot.y = -1.0; } 
    if(IsKeyDown(KEY_D)) { mov.y = +1.0; rot.x = +1.0; }
    if(IsKeyDown(KEY_A)) { mov.y = -1.0; rot.x = -1.0; }
    if(IsKeyDown(KEY_SPACE)) { mov.z = 1.0;}

    // apply gravity
    //if(player.pos.z > EPSILON)
    //    player.vel.z -= GRAVITY*dt;

    // update position
    //Vector3 pos_delta = Vector3Scale(player.vel, player.speed*dt);
    //player.pos = Vector3Add(player.pos, pos_delta);

    float camera_mov_speed = player.speed*dt;
    float camera_rot_speed = CAMERA_ROTATION_SPEED*dt;

    mov = Vector3Scale(mov, camera_mov_speed);
    rot = Vector3Scale(rot, camera_rot_speed);

    Vector2 mouse_delta = GetMouseDelta();

    if(ABS(mouse_delta.x) < 255.0f && ABS(mouse_delta.y) < 255.0f) // to avoid wild mouse movement on window resize
    {
        rot.x += mouse_delta.x*CAMERA_MOUSE_MOVE_SENSITIVITY;
        rot.y += mouse_delta.y*CAMERA_MOUSE_MOVE_SENSITIVITY;
    }

    float zoom = 0.0;

    bool lockView = true;
    bool rotateAroundTarget = false;
    bool rotateUp = false;
    bool moveInWorldPlane = true;

    // Camera rotation
    CameraPitch(&camera, -rot.y, lockView, rotateAroundTarget, rotateUp);
    CameraYaw(&camera, -rot.x, rotateAroundTarget);
    CameraRoll(&camera, rot.z);

    // Camera movement
    CameraMoveForward(&camera, mov.x, moveInWorldPlane);
    CameraMoveRight(&camera, mov.y, moveInWorldPlane);
    CameraMoveUp(&camera, mov.z);

    // Zoom target distance
    CameraMoveToTarget(&camera, zoom);

}

void player_draw()
{
    Vector3 pos = (Vector3){camera.position.x, camera.position.y, camera.position.z};
    DrawModelEx(girl, camera.target, (Vector3) {0.0,1.0,0.0}, 180.0, (Vector3){1.0,1.0,1.0}, WHITE);
}
