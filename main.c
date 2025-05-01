
#include <stdio.h>
#include "common.h"
#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"

#include "player.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------

#define CAMERA_MOVE_SPEED                               5.4f
#define CAMERA_ROTATION_SPEED                           0.03f
#define CAMERA_PAN_SPEED                                0.2f
#define CAMERA_MOUSE_MOVE_SENSITIVITY                   0.003f

#define MAX_COLUMNS 20
float heights[MAX_COLUMNS] = { 0 };
Vector3 positions[MAX_COLUMNS] = { 0 };
Color colors[MAX_COLUMNS] = { 0 };

Camera camera = {0};
Model girl;

void update();
void draw();

int main(void)
{
    const int screenWidth = 1200;
    const int screenHeight = 800;

    unsigned int flags = (FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(flags);

    InitWindow(screenWidth, screenHeight, "Rekt");
    MaximizeWindow();

    camera.position = (Vector3){ 0.0f, 2.0f, 4.0f };
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();

    float rate = GetMonitorRefreshRate(GetCurrentMonitor());
    SetTargetFPS(rate);

    // Generates some random columns
    for (int i = 0; i < MAX_COLUMNS; i++)
    {
        heights[i] = (float)GetRandomValue(1, 12);
        positions[i] = (Vector3){ (float)GetRandomValue(-15, 15), heights[i]/2.0f, (float)GetRandomValue(-15, 15) };
        colors[i] = (Color){ GetRandomValue(20, 255), GetRandomValue(10, 55), 30, 255 };
    }

    girl = LoadModel("models/female1.obj");
    Texture2D texture = LoadTexture("models/female1.png");
    girl.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    // Main game loop
    for(;;)
    {
        if(IsKeyPressed(KEY_Q)) break;

        update();
        draw();
    }

    CloseWindow();

    return 0;
}

void update()
{
    if(IsKeyPressed(KEY_ESCAPE))
    {
        if(IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }

    // player
    Vector3 mov = Vector3Zero();
    Vector3 rot = Vector3Zero();

    if(IsKeyDown(KEY_W)) { mov.x = +1.0; rot.y = +1.0; }
    if(IsKeyDown(KEY_S)) { mov.x = -1.0; rot.y = -1.0; } 
    if(IsKeyDown(KEY_D)) { mov.y = +1.0; rot.x = +1.0; }
    if(IsKeyDown(KEY_A)) { mov.y = -1.0; rot.x = -1.0; }
    if(IsKeyDown(KEY_SPACE)) { mov.z = +1.0;}

    float camera_mov_speed = CAMERA_MOVE_SPEED*GetFrameTime();
    float camera_rot_speed = CAMERA_ROTATION_SPEED*GetFrameTime();

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

void draw()
{
    BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

            DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 32.0f, 32.0f }, LIGHTGRAY); // Draw ground
            DrawCube((Vector3){ -16.0f, 2.5f, 0.0f }, 1.0f, 5.0f, 32.0f, BLUE);     // Draw a blue wall
            DrawCube((Vector3){ 16.0f, 2.5f, 0.0f }, 1.0f, 5.0f, 32.0f, LIME);      // Draw a green wall
            DrawCube((Vector3){ 0.0f, 2.5f, 16.0f }, 32.0f, 5.0f, 1.0f, GOLD);      // Draw a yellow wall

            // Draw some cubes around
            for (int i = 0; i < MAX_COLUMNS; i++)
            {
                DrawCube(positions[i], 2.0f, heights[i], 2.0f, colors[i]);
                DrawCubeWires(positions[i], 2.0f, heights[i], 2.0f, MAROON);
            }

            //DrawModelEx(girl, camera.target, (Vector3) {0.0,1.0,0.0}, 180.0, (Vector3){1.0,1.0,1.0}, WHITE);

        EndMode3D();

        // Draw info boxes
        DrawRectangle(600, 5, 195, 100, Fade(SKYBLUE, 0.5f));
        DrawRectangleLines(600, 5, 195, 100, BLUE);

        DrawText("Camera status:", 610, 15, 10, BLACK);
        DrawText(TextFormat("- Position: (%06.3f, %06.3f, %06.3f)", camera.position.x, camera.position.y, camera.position.z), 610, 30, 10, BLACK);
        DrawText(TextFormat("- Target: (%06.3f, %06.3f, %06.3f)", camera.target.x, camera.target.y, camera.target.z), 610, 45, 10, BLACK);
        DrawText(TextFormat("- Up: (%06.3f, %06.3f, %06.3f)", camera.up.x, camera.up.y, camera.up.z), 610, 60, 10, BLACK);

    EndDrawing();
}
