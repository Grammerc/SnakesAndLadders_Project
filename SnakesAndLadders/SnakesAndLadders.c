#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define BOARD_ROWS 10
#define BOARD_COLS 10

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snakes and Ladders");
    ToggleFullscreen();

    SetTargetFPS(60);

    int cellWidth = SCREEN_WIDTH / BOARD_COLS;
    int cellHeight = SCREEN_HEIGHT / BOARD_ROWS;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int row = 0; row < BOARD_ROWS; row++) {
            for (int col = 0; col < BOARD_COLS; col++) {
                int x = col * cellWidth;
                int y = row * cellHeight;
                DrawRectangleLines(x, y, cellWidth, cellHeight, BLACK);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}