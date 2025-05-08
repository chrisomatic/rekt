#include <stdio.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "common.h"
#include "player.h"

#define CAMERA_ROTATION_SPEED   0.03
#define MOUSE_MOVE_SENSITIVITY  0.003

Player player = {0};
Camera camera = {0};
Model girl;
Model greenman;

ModelAnimation *modelAnimations;
int animsCount = 0;
unsigned int animIndex = 2;
unsigned int animCurrentFrame = 0;

void player_init()
{
    player.pos = (Vector3){ 0.0, 0.0, 0.0 };
    player.pos_prior = (Vector3){ 0.0, 0.0, 0.0 };
    player.target = (Vector3){ 0.0, 0.0, 1.0 };

    player.run_speed = 4.0; //
    player.jump_speed = 4.0; // m/s
    player.height = 1.6;
    player.facing_angle = 0.0;

    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    camera.position = player.pos;
    camera.position.y += (player.height*0.90); // eye level
    camera.target = camera.position; 
    camera.target.z += 1.0;

    girl = LoadModel("models/female1.obj");
    Texture2D texture = LoadTexture("models/female1.png");
    girl.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    greenman = LoadModel("models/greenman.glb");
    modelAnimations = LoadModelAnimations("models/greenman.glb", &animsCount);
}

void player_update(float dt)
{
    // update velocity

#if 0
    Vector3 fwd = Vector3Normalize(Vector3Subtract(player.target, player.pos));
    Vector3 up = (Vector3){ 0.0, 1.0, 0.0 };
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, up));
#else
    Vector3 fwd = GetCameraForward(&camera);
    Vector3 right = GetCameraRight(&camera);
#endif

    fwd.y = 0.0;
    right.y = 0.0;

    player.vel.x = 0.0;
    player.vel.z = 0.0;

    if(IsKeyDown(KEY_W)) { player.vel = Vector3Add(player.vel,fwd); }
    if(IsKeyDown(KEY_S)) { player.vel = Vector3Subtract(player.vel,fwd); } 
    if(IsKeyDown(KEY_D)) { player.vel = Vector3Add(player.vel,right); }
    if(IsKeyDown(KEY_A)) { player.vel = Vector3Subtract(player.vel, right); }

    player.vel.x *= player.run_speed;
    player.vel.z *= player.run_speed;

    if(IsKeyDown(KEY_SPACE)) { 
        if(player.pos.y <= EPSILON)
            player.vel.y = player.jump_speed;
    }

    if(IsKeyPressed(KEY_F2)) { g_debug = !g_debug; }

    // apply gravity
    if(player.pos.y > EPSILON)
        player.vel.y -= (GRAVITY*dt);
    
    // update position
    player.pos_prior = player.pos;
    Vector3 pos_delta = Vector3Scale(player.vel, dt);
    player.pos = Vector3Add(player.pos, pos_delta);

    if(player.pos.y < 0)
    {
        player.pos.y = 0.0;
        player.vel.y = 0.0;
    }

    // update rotation

    Vector3 rot = Vector3Zero();
    rot = Vector3Scale(rot, CAMERA_ROTATION_SPEED*dt);

    Vector2 mouse_delta = GetMouseDelta();

    if(ABS(mouse_delta.x) < 255.0f && ABS(mouse_delta.y) < 255.0f) // to avoid wild mouse movement on window resize
    {
        rot.x += mouse_delta.x*MOUSE_MOVE_SENSITIVITY;
        rot.y += mouse_delta.y*MOUSE_MOVE_SENSITIVITY;
    }

    player.facing_angle -= RAD2DEG*rot.x;

    Vector3 targetPosition = Vector3Subtract(player.target, player.pos);
    targetPosition = Vector3RotateByAxisAngle(targetPosition, (Vector3){0.0,1.0,0.0}, -rot.x);
    player.target = Vector3Add(player.pos, targetPosition);

    // update camera

    // rotation
    CameraYaw(&camera, -rot.x, false);
    CameraPitch(&camera, -rot.y, true, false, false);
    CameraRoll(&camera, rot.z);

    // position
    Vector3 delta_pos = Vector3Subtract(player.pos, player.pos_prior);
    camera.position = Vector3Add(camera.position, delta_pos);
    player.target = Vector3Add(player.target, delta_pos);
    camera.target = Vector3Add(camera.target, delta_pos);

    ModelAnimation anim = modelAnimations[animIndex];
    animCurrentFrame = (animCurrentFrame + 1)%anim.frameCount;
    UpdateModelAnimation(greenman, anim, animCurrentFrame);
}

void player_draw()
{

    ModelAnimation anim = modelAnimations[animIndex];
    UpdateModelAnimation(greenman, anim, animCurrentFrame);
    DrawModelEx(greenman, player.pos, (Vector3) {0.0,1.0,0.0}, player.facing_angle, (Vector3){1.0,1.0,1.0}, WHITE);

    DrawModelEx(girl, (Vector3) {2.0,0.0,2.0}, (Vector3) {0.0,1.0,0.0}, 0.0, (Vector3){1.0,1.0,1.0}, WHITE);

    if(g_debug)
    {
        DrawSphere(camera.target, 0.2, PINK);
        DrawSphereWires(player.target, 0.2, 10, 10, ORANGE);
    }
}

