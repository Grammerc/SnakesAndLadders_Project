#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SCREEN_WIDTH   1920
#define SCREEN_HEIGHT  1080
#define BOARD_SIZE     10
#define CELL_SIZE      80
#define BOARD_OFFSET_X (SCREEN_WIDTH - BOARD_SIZE * CELL_SIZE) / 2
#define BOARD_OFFSET_Y 150
#define MAX_SNAKES     5
#define MAX_LADDERS    5

typedef enum {
    TITLE_SCREEN,
    SELECT_PLAYERS,
    ENTER_NAMES,
    GAME_ACTIVE,
    DICE_ROLLING,
    PIECE_MOVING,
    NAME_INPUT_SAVE,
    GAME_OVER
}GameState;

typedef struct { Texture2D texture; Rectangle bounds; }Button;

typedef struct {
    int     position;
    Color   color;
    char    name[32];
    int     playerNumber;
    Texture2D token;
}Player;

typedef struct { int start, end; }SnakeOrLadder;

/* ────────── globals */
GameState state = TITLE_SCREEN;
Player   players[4];
int      playerCount = 2;
int      diceCount = 1;          /* 1 or 2 dice */
int      currentPlayer = 0;
int      diceTotal = 1;
int      dieA = 1, dieB = 0;             /* faces for drawing */
float    diceTimer = 0.0f;

int   boardNumbers[BOARD_SIZE][BOARD_SIZE];
SnakeOrLadder snakes[MAX_SNAKES];
SnakeOrLadder ladders[MAX_LADDERS];

Texture2D diceTex[6];
Texture2D tokenTex[4];

/* name-entry helpers */
char nameBuf[32] = ""; int nameLen = 0; int nameIdx = 0;

/* piece-moving */
int   stepsRemaining = 0, stepDir = 1; float stepTimer = 0, STEP_DELAY = 0.15f;

/* save */
char  saveFile[64] = ""; int saveLen = 0; GameState returnState;

/* ────────── utility */
Button makeBtn(const char* p, int x, int y) {
    Button b; b.texture = LoadTexture(p); b.bounds = (Rectangle){ x,y,b.texture.width,b.texture.height }; return b;
}
bool hit(Button b) { return CheckCollisionPointRec(GetMousePosition(), b.bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON); }

/* ────────── board setup */
void InitBoard() {
    int n = 1; bool right = 1;
    for (int r = BOARD_SIZE - 1; r >= 0; r--) {
        if (right) for (int c = 0; c < BOARD_SIZE; c++)      boardNumbers[r][c] = n++;
        else      for (int c = BOARD_SIZE - 1; c >= 0; c--)   boardNumbers[r][c] = n++;
        right = !right;
    }
    snakes[0] = (SnakeOrLadder){ 16,6 };  snakes[1] = (SnakeOrLadder){ 37,21 };
    snakes[2] = (SnakeOrLadder){ 49,27 }; snakes[3] = (SnakeOrLadder){ 82,55 };
    snakes[4] = (SnakeOrLadder){ 97,78 };
    ladders[0] = (SnakeOrLadder){ 4,14 }; ladders[1] = (SnakeOrLadder){ 8,30 };
    ladders[2] = (SnakeOrLadder){ 28,76 }; ladders[3] = (SnakeOrLadder){ 71,92 };
    ladders[4] = (SnakeOrLadder){ 80,99 };
}
Vector2 CellPos(int num) {
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (boardNumbers[r][c] == num)
                return (Vector2) {
                BOARD_OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2,
                    BOARD_OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2
            };
    return (Vector2) { 0, 0 };
}
int Slide(int pos) {
    for (int i = 0; i < MAX_SNAKES; i++) if (snakes[i].start == pos) return snakes[i].end;
    for (int i = 0; i < MAX_LADDERS; i++) if (ladders[i].start == pos) return ladders[i].end;
    return pos;
}

/* ────────── dice */
int RollDice() {
    dieA = GetRandomValue(1, 6);
    if (diceCount == 1) { dieB = 0; return dieA; }
    dieB = GetRandomValue(1, 6);
    return dieA + dieB;
}

/* ────────── draw */
void DrawBoard() {
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++) {
            Color col = ((r + c) % 2) ? WHITE : LIGHTGRAY;
            DrawRectangle(BOARD_OFFSET_X + c * CELL_SIZE, BOARD_OFFSET_Y + r * CELL_SIZE, CELL_SIZE, CELL_SIZE, col);
            char t[4]; sprintf(t, "%d", boardNumbers[r][c]);
            DrawText(t, BOARD_OFFSET_X + c * CELL_SIZE + 8, BOARD_OFFSET_Y + r * CELL_SIZE + 6, 18, DARKGRAY);
            DrawRectangleLines(BOARD_OFFSET_X + c * CELL_SIZE, BOARD_OFFSET_Y + r * CELL_SIZE, CELL_SIZE, CELL_SIZE, BLACK);
        }
    for (int i = 0; i < MAX_SNAKES; i++) DrawLineEx(CellPos(snakes[i].start), CellPos(snakes[i].end), 5, RED);
    for (int i = 0; i < MAX_LADDERS; i++) DrawLineEx(CellPos(ladders[i].start), CellPos(ladders[i].end), 5, GREEN);
}
void DrawPlayers() {
    for (int i = 0; i < playerCount; i++) {
        Vector2 p = CellPos(players[i].position);
        float r = CELL_SIZE / 4.f;
        p.x += cosf(2 * PI * i / playerCount) * r * 0.6f;
        p.y += sinf(2 * PI * i / playerCount) * r * 0.6f;
        DrawTexture(players[i].token, p.x - players[i].token.width / 2, p.y - players[i].token.height / 2, WHITE);
    }
}

/* ────────── save / load (unchanged simple binary) */
void SaveFile(const char* f) {
    FILE* fp = fopen(f, "wb"); if (!fp) return;
    fwrite(&playerCount, sizeof(int), 1, fp);
    fwrite(&currentPlayer, sizeof(int), 1, fp);
    fwrite(&diceTotal, sizeof(int), 1, fp);
    fwrite(players, sizeof(Player), playerCount, fp);
    fclose(fp);
}

/* ────────── buttons / assets */
Button bg, titleB, startB, loadB, exitB;
Button twoP, threeP, fourP, oneDie, twoDice;
Button rollB, throwB, leaveB;

void LoadAssets() {
    bg = makeBtn("assets/buttons/background.png", SCREEN_WIDTH / 2 - 960, 0);
    titleB = makeBtn("assets/buttons/title.png", SCREEN_WIDTH / 2 - 400, 150);
    startB = makeBtn("assets/buttons/start_btn.png", SCREEN_WIDTH / 2 - 100, 690);
    loadB = makeBtn("assets/buttons/load_btn.png", SCREEN_WIDTH / 2 - 100, 780);
    exitB = makeBtn("assets/buttons/exit_btn.png", SCREEN_WIDTH / 2 - 100, 860);

    twoP = makeBtn("assets/buttons/2p.png", SCREEN_WIDTH / 2 - 260, 500);
    threeP = makeBtn("assets/buttons/3p.png", SCREEN_WIDTH / 2 - 60, 500);
    fourP = makeBtn("assets/buttons/4p.png", SCREEN_WIDTH / 2 + 140, 500);
    oneDie = makeBtn("assets/buttons/one_die_btn.png", SCREEN_WIDTH / 2 - 200, 650);
    twoDice = makeBtn("assets/buttons/two_dice_btn.png", SCREEN_WIDTH / 2 + 40, 650);

    rollB = makeBtn("assets/buttons/roll_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    throwB = makeBtn("assets/buttons/throw_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    leaveB = makeBtn("assets/buttons/exit_btn.png", 50, 400);

    for (int i = 0; i < 6; i++) { char p[64]; sprintf(p, "assets/dice/dice%d.png", i + 1); diceTex[i] = LoadTexture(p); }
    for (int i = 0; i < 4; i++) { char p[64]; sprintf(p, "assets/tokens/token%d.png", i + 1); tokenTex[i] = LoadTexture(p); }
}

/* ────────── main */
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snakes & Ladders");
    SetTargetFPS(60);
    srand((unsigned)time(NULL));
    InitBoard();
    SetTraceLogLevel(LOG_ALL);
    LoadAssets();

    for (int i = 0; i < 4; i++) {
        players[i].position = 1;
        players[i].color = (Color){ rand() % 256,rand() % 256,rand() % 256,255 };
        players[i].playerNumber = i + 1;
        players[i].token = tokenTex[i];
        sprintf(players[i].name, "Player %d", i + 1);
    }

    while (!WindowShouldClose()) {
        /* ─── logic ───────────────── */
        switch (state) {
        case TITLE_SCREEN:
            if (hit(startB)) state = SELECT_PLAYERS;
            if (hit(exitB))  CloseWindow();
            break;

        case SELECT_PLAYERS:
            if (hit(twoP))   playerCount = 2;
            if (hit(threeP)) playerCount = 3;
            if (hit(fourP))  playerCount = 4;

            if (hit(oneDie)) { diceCount = 1; state = ENTER_NAMES; nameIdx = 0; nameLen = 0; nameBuf[0] = 0; }
            if (hit(twoDice)) { diceCount = 2; state = ENTER_NAMES; nameIdx = 0; nameLen = 0; nameBuf[0] = 0; }

            if (hit(leaveB)) state = TITLE_SCREEN;
            break;

        case ENTER_NAMES: {
            int ch = GetCharPressed();
            while (ch > 0) {
                if (ch >= 32 && ch <= 126 && nameLen < 31) { nameBuf[nameLen++] = (char)ch; nameBuf[nameLen] = 0; }
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && nameLen > 0) { nameBuf[--nameLen] = 0; }
            if (IsKeyPressed(KEY_ENTER) && nameLen > 0) {
                strncpy(players[nameIdx].name, nameBuf, 31); players[nameIdx].name[31] = 0;
                nameIdx++; nameLen = 0; nameBuf[0] = 0;
                if (nameIdx == playerCount) state = GAME_ACTIVE;
            }
            if (IsKeyPressed(KEY_ESCAPE)) state = TITLE_SCREEN;
        }break;

        case GAME_ACTIVE:
            if (hit(rollB)) { diceTotal = RollDice(); state = DICE_ROLLING; }
            if (hit(leaveB)) state = TITLE_SCREEN;
            break;

        case DICE_ROLLING:
            if (hit(throwB)) {
                stepsRemaining = diceTotal; stepDir = 1; stepTimer = 0; state = PIECE_MOVING;
            }
            break;

        case PIECE_MOVING:
            stepTimer += GetFrameTime();
            if (stepTimer >= STEP_DELAY) {
                stepTimer = 0;
                players[currentPlayer].position += stepDir;
                stepsRemaining--;
                if (stepsRemaining == 0) {
                    if (players[currentPlayer].position > 100) players[currentPlayer].position = 100 - (players[currentPlayer].position - 100);
                    players[currentPlayer].position = Slide(players[currentPlayer].position);
                    if (players[currentPlayer].position == 100) state = GAME_OVER;
                    else { currentPlayer = (currentPlayer + 1) % playerCount; state = GAME_ACTIVE; }
                }
            }
            break;

        case GAME_OVER:
            if (IsKeyPressed(KEY_ENTER)) {
                for (int i = 0; i < playerCount; i++) players[i].position = 1;
                currentPlayer = 0; state = TITLE_SCREEN;
            }
            break;
        }

        /* ─── draw ───────────────── */
        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (state) {
        case TITLE_SCREEN:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            DrawTexture(titleB.texture, titleB.bounds.x, titleB.bounds.y, WHITE);
            DrawTexture(startB.texture, startB.bounds.x, startB.bounds.y, WHITE);
            DrawTexture(exitB.texture, exitB.bounds.x, exitB.bounds.y, WHITE);
            break;

        case SELECT_PLAYERS:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            DrawText("Choose players and dice", SCREEN_WIDTH / 2 - 250, 420, 40, BLACK);
            DrawTexture(twoP.texture, twoP.bounds.x, twoP.bounds.y, WHITE);
            DrawTexture(threeP.texture, threeP.bounds.x, threeP.bounds.y, WHITE);
            DrawTexture(fourP.texture, fourP.bounds.x, fourP.bounds.y, WHITE);
            DrawTexture(oneDie.texture, oneDie.bounds.x, oneDie.bounds.y, WHITE);
            DrawTexture(twoDice.texture, twoDice.bounds.x, twoDice.bounds.y, WHITE);
            DrawTexture(leaveB.texture, leaveB.bounds.x, leaveB.bounds.y, WHITE);
            break;

        case ENTER_NAMES:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            char prompt[64]; sprintf(prompt, "Enter name for Player %d", nameIdx + 1);
            DrawText(prompt, SCREEN_WIDTH / 2 - 220, 420, 40, BLACK);
            DrawRectangleLines(SCREEN_WIDTH / 2 - 250, 480, 500, 60, BLACK);
            DrawText(nameBuf, SCREEN_WIDTH / 2 - 240, 495, 40, BLACK);
            break;

        case GAME_ACTIVE:
            DrawBoard(); DrawPlayers();
            DrawTexture(rollB.texture, rollB.bounds.x, rollB.bounds.y, WHITE);
            DrawTexture(leaveB.texture, leaveB.bounds.x, leaveB.bounds.y, WHITE);
            if (diceCount == 1) DrawTexture(diceTex[dieA - 1], SCREEN_WIDTH - 200, 100, WHITE);
            else {
                DrawTexture(diceTex[dieA - 1], SCREEN_WIDTH - 240, 100, WHITE);
                DrawTexture(diceTex[dieB - 1], SCREEN_WIDTH - 160, 100, WHITE);
            }
            DrawText(players[currentPlayer].name, 50, 100, 30, players[currentPlayer].color);
            break;

        case DICE_ROLLING:
            DrawBoard(); DrawPlayers();
            DrawTexture(throwB.texture, throwB.bounds.x, throwB.bounds.y, WHITE);
            if (diceCount == 1) DrawTexture(diceTex[dieA - 1], SCREEN_WIDTH - 200, 100, WHITE);
            else {
                DrawTexture(diceTex[dieA - 1], SCREEN_WIDTH - 240, 100, WHITE);
                DrawTexture(diceTex[dieB - 1], SCREEN_WIDTH - 160, 100, WHITE);
            }
            DrawText(players[currentPlayer].name, 50, 100, 30, players[currentPlayer].color);
            break;

        case PIECE_MOVING:
            DrawBoard(); DrawPlayers();
            DrawTexture(leaveB.texture, leaveB.bounds.x, leaveB.bounds.y, WHITE);
            if (diceCount == 1) DrawTexture(diceTex[dieA - 1], SCREEN_WIDTH - 200, 100, WHITE);
            else {
                DrawTexture(diceTex[dieA - 1], SCREEN_WIDTH - 240, 100, WHITE);
                DrawTexture(diceTex[dieB - 1], SCREEN_WIDTH - 160, 100, WHITE);
            }
            break;

        case GAME_OVER:
            DrawText("GAME OVER", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 60, 50, RED);
            DrawText("Press ENTER", SCREEN_WIDTH / 2 - 110, SCREEN_HEIGHT / 2 + 20, 30, BLACK);
            break;
        }

        EndDrawing();
    }

    for (int i = 0; i < 6; i++) UnloadTexture(diceTex[i]);
    for (int i = 0; i < 4; i++) UnloadTexture(tokenTex[i]);
    CloseWindow();
    return 0;
}