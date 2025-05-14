#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "player.h"
#include "sky.h"
#include "lights.h"
#include "terrain.h"
#include "gui_styles/style_cyber.h"


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------

const int screen_width = 1200;
const int screen_height = 800;

bool g_editor = false;
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
    unsigned int flags = (FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(flags);

    InitWindow(screen_width, screen_height, "Rekt");
    MaximizeWindow();

    DisableCursor();

    float rate = GetMonitorRefreshRate(GetCurrentMonitor());
    SetTargetFPS(rate);

    GuiLoadStyleCyber();

    player_init();
    terrain_init();
    sky_init();
}

void update()
{
    if(IsKeyPressed(KEY_ESCAPE))
    {
        if(IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }

    if(IsKeyPressed(KEY_F2)) {
        g_editor = !g_editor;
        if(g_editor) EnableCursor();
        else DisableCursor();
    }

    float dt = GetFrameTime();

    lights_update();
    terrain_update();
    player_update(dt);
}

void draw()
{
    BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

            sky_draw();
            BeginShaderMode(lights_shader);
                terrain_draw();
                player_draw();
            EndShaderMode();
            DrawGrid(32, 1.0f);

        EndMode3D();

        if(g_editor)
        {
            int w = GetRenderWidth();
            int h = GetRenderHeight();

            static float value = 0.5f;
            Rectangle r = {(w-800)/2.0, (h-600)/2.0, 800, 600};

            GuiWindowBox(r, "Editor");
            GuiCheckBox((Rectangle){ r.x+50, r.y+40, 16, 16}, TextFormat("Debug"), &g_debug);
            GuiSlider((Rectangle){ r.x+50, r.y+70, 216, 16 }, TextFormat("Planar %0.2f", terrain_scale_planar), NULL, &terrain_scale_planar, 0.0f, 5.0f);
            GuiSlider((Rectangle){ r.x+50, r.y+100, 216, 16 }, TextFormat("Height %0.2f", terrain_scale_height), NULL, &terrain_scale_height, 0.0f, 20.0f);
        }

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
        DrawText(TextFormat("F2      - Toggle debug mode", player.angle_theta, player.angle_omega), 10, 180, 20, tcolor);

    EndDrawing();
}
