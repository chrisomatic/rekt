#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "raylib.h"
#include "raymath.h"
#include "player.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------

#define MAX_COLUMNS 20
float heights[MAX_COLUMNS] = { 0 };
Vector3 positions[MAX_COLUMNS] = { 0 };
Color colors[MAX_COLUMNS] = { 0 };

void init();
void update();
void draw();

int main(void)
{
    init();

    for(;;)
    {
        if(IsKeyPressed(KEY_Q))
            break;

        update();
        draw();
    }

    CloseWindow();

    return 0;
}

void init()
{
    const int screenWidth = 1200;
    const int screenHeight = 800;

    unsigned int flags = (FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(flags);

    InitWindow(screenWidth, screenHeight, "Rekt");
    MaximizeWindow();

    DisableCursor();

    float rate = GetMonitorRefreshRate(GetCurrentMonitor());
    SetTargetFPS(rate);

    // Generates some random columns
    for(int i = 0; i < MAX_COLUMNS; i++)
    {
        heights[i] = (float)GetRandomValue(1, 12);
        positions[i] = (Vector3){ (float)GetRandomValue(-15, 15), heights[i]/2.0f, (float)GetRandomValue(-15, 15) };
        colors[i] = (Color){ GetRandomValue(20, 255), GetRandomValue(10, 55), 30, 255 };
    }

    player_init();

}

void update()
{
    if(IsKeyPressed(KEY_ESCAPE))
    {
        if(IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }

    float dt = GetFrameTime();

    player_update(dt);
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


            player_draw();

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
