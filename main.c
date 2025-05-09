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

bool g_debug = false;

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

    unsigned int flags = (FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
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

            DrawCube((Vector3){ -16.0f, 2.5f, 0.0f }, 1.0f, 5.0f, 32.0f, BLUE);     // Draw a blue wall
            DrawCube((Vector3){ 16.0f, 2.5f, 0.0f }, 1.0f, 5.0f, 32.0f, LIME);      // Draw a green wall
            DrawCube((Vector3){ 0.0f, 2.5f, 16.0f }, 32.0f, 5.0f, 1.0f, GOLD);      // Draw a yellow wall

            // Draw some cubes around
            for (int i = 0; i < MAX_COLUMNS; i++)
            {
                if(g_debug) DrawCubeWires(positions[i], 2.0f, heights[i], 2.0f, MAROON);
                else        DrawCube(positions[i], 2.0f, heights[i], 2.0f, colors[i]);
            }

            player_draw();
            DrawGrid(32, 1.0f);

        EndMode3D();

        // Draw info boxes
        DrawRectangle(5, 5, 400, 200, Fade(DARKBLUE, 0.7f));

        Color tcolor = WHITE;
        DrawText("Player status:", 10, 15, 20, tcolor);
        DrawText(TextFormat("- Position: (%06.3f, %06.3f, %06.3f)", player.pos.x, player.pos.y, player.pos.z), 10, 40, 20, tcolor);
        DrawText(TextFormat("- Vel:      (%06.3f, %06.3f, %06.3f)", player.vel.x, player.vel.y, player.vel.z), 10, 60, 20, tcolor);
        DrawText(TextFormat("- Angles:   (%06.3f, %06.3f)", player.angle_theta, player.angle_omega), 10, 80, 20, tcolor);
        DrawText(TextFormat("W,A,S,D - Move", player.angle_theta, player.angle_omega), 10, 120, 20, tcolor);
        DrawText(TextFormat("Space   - Jump", player.angle_theta, player.angle_omega), 10, 140, 20, tcolor);
        DrawText(TextFormat("Tab     - Toggle viewpoint", player.angle_theta, player.angle_omega), 10, 160, 20, tcolor);

    EndDrawing();
}
