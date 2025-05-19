#include "common.h"
#include "raylib.h"
#include "raymath.h"
#include "lights.h"
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
    player.target = (Vector3){ 0.0, 0.0, 1.0 };

    player.viewpoint = VIEWPOINT_FIRST;
    player.run_speed = 5.0; // m/s
    player.jump_speed = 4.0; // m/s
    player.height = 1.0;
    player.angle_theta = 0.0;

    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    girl = LoadModel("models/female1.obj");
    Texture2D texture = LoadTexture("models/female1.png");
    girl.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    girl.materials[0].shader = lights_shader;

    greenman = LoadModel("models/greenman.glb");
    modelAnimations = LoadModelAnimations("models/greenman.glb", &animsCount);
    greenman.materials[0].shader = lights_shader;
    greenman.materials[1].shader = lights_shader;
}

void player_update(float dt)
{
    if(g_editor)
        return;

    // update velocity

    Vector3 fwd = Vector3Normalize(Vector3Subtract(player.target, player.pos));
    Vector3 up = (Vector3){ 0.0, 1.0, 0.0 };
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, up));

    fwd.y = 0.0;
    right.y = 0.0;

    float ground_y = terrain_get_ground(player.pos.x, player.pos.z, &player.ground);
    bool on_ground = player.pos.y <= ground_y + GROUND_EPSILON;
    
    if(on_ground)
    {
        player.vel.x = 0.0;
        player.vel.z = 0.0;
        player.running = false;

        if(IsKeyDown(KEY_LEFT_SHIFT)) { player.running = true; };
        if(IsKeyDown(KEY_W)) { player.vel = Vector3Add(player.vel,fwd); }
        if(IsKeyDown(KEY_S)) { player.vel = Vector3Subtract(player.vel,fwd); } 
        if(IsKeyDown(KEY_D)) { player.vel = Vector3Add(player.vel,right); }
        if(IsKeyDown(KEY_A)) { player.vel = Vector3Subtract(player.vel, right); }

        if(player.vel.x != 0.0 && player.vel.z != 0.0)
        {
            // handle diagonal movement
            player.vel.x *= 0.7071;
            player.vel.z *= 0.7071;
        }

        player.vel.x *= player.run_speed;
        player.vel.z *= player.run_speed;

        if(player.running)
        {
            player.vel.x *= 5.0;
            player.vel.z *= 5.0;
        }
    }

    if(IsKeyDown(KEY_SPACE)) { 
        if(on_ground)
            player.vel.y = player.jump_speed;
    }

    if(IsKeyPressed(KEY_TAB)) { g_debug = !g_debug; }
    if(IsKeyPressed(KEY_P)) {
        if(player.viewpoint == VIEWPOINT_FIRST) player.viewpoint = VIEWPOINT_THIRD;
        else player.viewpoint = VIEWPOINT_FIRST;
    }

    // apply gravity
    if(player.pos.y > ground_y + EPSILON)
        player.vel.y -= (GRAVITY*dt);
    
    // update position
    Vector3 pos_delta = Vector3Scale(player.vel, dt);
    player.pos = Vector3Add(player.pos, pos_delta);

    if(player.pos.y < ground_y)
    {
        player.pos.y = ground_y;
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

    player.angle_theta -= RAD2DEG*rot.x;
    player.angle_theta = fmod(player.angle_theta,360.0f);

    player.angle_omega -= RAD2DEG*rot.y;
    if(player.angle_omega < -55.0) player.angle_omega = -55.0;
    else if(player.angle_omega > +55.0) player.angle_omega = +55.0;

    Vector3 target = {0.0,0.0,1.0};
    target = Vector3RotateByAxisAngle(target, up, DEG2RAD*player.angle_theta);
    target = Vector3RotateByAxisAngle(target, right, DEG2RAD*player.angle_omega);
    target = Vector3Add(player.pos, target);
    player.target = target;

    // update camera

    if(player.viewpoint == VIEWPOINT_FIRST)
    {
        Vector3 offset = (Vector3) {0.0, player.height * 0.80, 0.0 };
        camera.position = Vector3Add(player.pos, offset);
        camera.target   = Vector3Add(player.target, offset);
    }
    else
    {
        Vector3 offset = (Vector3) {0.0, player.height * 0.80 + 1.0, 0.0 };
        camera.position = Vector3Add(player.pos, offset);
        camera.target   = Vector3Add(player.target, offset);

        camera.position = Vector3Subtract(camera.position, Vector3Scale(fwd,3.0));
    }

    // update animation frame

    ModelAnimation anim = modelAnimations[animIndex];

    if(on_ground && Vector3Length(player.vel) > 0.0)
    {
        animCurrentFrame = (animCurrentFrame + 1)%anim.frameCount;
        UpdateModelAnimation(greenman, anim, animCurrentFrame);
    }
    else
    {
        animCurrentFrame = 0;
        UpdateModelAnimation(greenman, anim, animCurrentFrame);
    }
}

void player_draw()
{
    DrawModelEx(greenman, player.pos, (Vector3) {0.0,1.0,0.0}, player.angle_theta, (Vector3){0.5,0.5,0.5}, WHITE);
    DrawModelEx(girl, (Vector3) {0.0,0.0,0.0}, (Vector3) {0.0,1.0,0.0}, 180.0, (Vector3){1.0,1.0,1.0}, WHITE);

    if(g_debug)
    {
        DrawSphereWires(camera.target, 0.1, 10, 10,  PINK);
        DrawSphereWires(player.target, 0.1, 10, 10, ORANGE);

        Ground ground = {0};
        terrain_get_ground(player.pos.x, player.pos.z, &ground);
        DrawSphere(ground.a, 0.06, RED);
        DrawSphere(ground.b, 0.06, GREEN);
        DrawSphere(ground.c, 0.06, BLUE);
    }
}

