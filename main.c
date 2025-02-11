#include "raylib.h"

int main(void)
{
    InitWindow(1024, 800, "REKT");

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Hi Kam! I'm a Game", 380, 350, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
