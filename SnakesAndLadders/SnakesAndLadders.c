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

typedef enum
{
    TITLE_SCREEN,
    SELECT_PLAYERS,
    GAME_ACTIVE,
    DICE_ROLLING,
    PIECE_MOVING,
    NAME_INPUT_SAVE,
    GAME_OVER
} GameState;

typedef struct
{
    Texture2D texture;
    Rectangle bounds;
} Button;

typedef struct
{
    int   position;
    Color color;
    char  name[20];
    int   playerNumber;
} Player;

typedef struct
{
    int start;
    int end;
} SnakeOrLadder;

int boardNumbers[BOARD_SIZE][BOARD_SIZE];
SnakeOrLadder snakes[MAX_SNAKES];
SnakeOrLadder ladders[MAX_LADDERS];

int   stepsRemaining = 0;
int   stepDir = 1;
float stepTimer = 0.0f;
const float STEP_DELAY = 0.15f;

char  fileName[64] = "";
int   nameLen = 0;
GameState returnState = GAME_ACTIVE;

char  loadNames[64][64];
int   loadCount = 0;

Button createButton(const char* path, int x, int y)
{
    Button b;
    b.texture = LoadTexture(path);
    b.bounds = (Rectangle){ x,y,b.texture.width,b.texture.height };
    return b;
}

bool isButtonPressed(Button b)
{
    return CheckCollisionPointRec(GetMousePosition(), b.bounds) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

int rollDice() { return GetRandomValue(1, 6); }

void InitBoardNumbers()
{
    int num = 1; bool right = true;
    for (int r = BOARD_SIZE - 1; r >= 0; r--)
    {
        if (right)  for (int c = 0; c < BOARD_SIZE; c++)      boardNumbers[r][c] = num++;
        else       for (int c = BOARD_SIZE - 1; c >= 0; c--) boardNumbers[r][c] = num++;
        right = !right;
    }
}

void InitSnakesAndLadders()
{
    snakes[0] = (SnakeOrLadder){ 16,6 };   snakes[1] = (SnakeOrLadder){ 37,21 };
    snakes[2] = (SnakeOrLadder){ 49,27 };  snakes[3] = (SnakeOrLadder){ 82,55 };
    snakes[4] = (SnakeOrLadder){ 97,78 };
    ladders[0] = (SnakeOrLadder){ 4,14 };  ladders[1] = (SnakeOrLadder){ 8,30 };
    ladders[2] = (SnakeOrLadder){ 28,76 }; ladders[3] = (SnakeOrLadder){ 71,92 };
    ladders[4] = (SnakeOrLadder){ 80,99 };
}

Vector2 GetCellPosition(int cell)
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (boardNumbers[r][c] == cell)
                return (Vector2) {
                BOARD_OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2,
                    BOARD_OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2
            };
    return (Vector2) { 0, 0 };
}

int CheckSnakeOrLadder(int p)
{
    for (int i = 0; i < MAX_SNAKES; i++) if (snakes[i].start == p) return snakes[i].end;
    for (int i = 0; i < MAX_LADDERS; i++) if (ladders[i].start == p) return ladders[i].end;
    return p;
}

void DrawPlayers(Player* pl, int cnt)
{
    for (int i = 0; i < cnt; i++)
    {
        if (pl[i].position < 1 || pl[i].position > 100) continue;
        Vector2 pos = GetCellPosition(pl[i].position);
        float   r = CELL_SIZE / 4.0f;
        float   ang = 2 * PI * i / cnt;
        pos.x += cosf(ang) * r * 0.6f;
        pos.y += sinf(ang) * r * 0.6f;
        DrawCircleV(pos, r, pl[i].color);
        char id[3]; sprintf(id, "%d", pl[i].playerNumber);
        DrawText(id, pos.x - 6, pos.y - 10, 18, BLACK);
    }
}

void DrawSnakesAndLadders()
{
    for (int i = 0; i < MAX_SNAKES; i++) DrawLineEx(GetCellPosition(snakes[i].start), GetCellPosition(snakes[i].end), 5, RED);
    for (int i = 0; i < MAX_LADDERS; i++) DrawLineEx(GetCellPosition(ladders[i].start), GetCellPosition(ladders[i].end), 5, GREEN);
}

void DrawBoard()
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            Color cell = ((r + c) % 2) ? WHITE : LIGHTGRAY;
            DrawRectangle(BOARD_OFFSET_X + c * CELL_SIZE, BOARD_OFFSET_Y + r * CELL_SIZE, CELL_SIZE, CELL_SIZE, cell);
            char num[4]; sprintf(num, "%d", boardNumbers[r][c]);
            DrawText(num, BOARD_OFFSET_X + c * CELL_SIZE + 10, BOARD_OFFSET_Y + r * CELL_SIZE + 10, 20, DARKGRAY);
            DrawRectangleLines(BOARD_OFFSET_X + c * CELL_SIZE, BOARD_OFFSET_Y + r * CELL_SIZE, CELL_SIZE, CELL_SIZE, BLACK);
        }
}

bool SaveGameFile(Player* p, int count, int current, int diceRes, const char* file)
{
    FILE* f = fopen(file, "wb");
    if (!f) return false;
    fwrite(&count, sizeof(int), 1, f);
    fwrite(&current, sizeof(int), 1, f);
    fwrite(&diceRes, sizeof(int), 1, f);
    fwrite(p, sizeof(Player), count, f);
    fclose(f);
    return true;
}

bool LoadGameFile(Player* p, int* count, int* current, int* diceRes, const char* file)
{
    FILE* f = fopen(file, "rb");
    if (!f) return false;
    if (fread(count, sizeof(int), 1, f) != 1) { fclose(f); return false; }
    fread(current, sizeof(int), 1, f);
    fread(diceRes, sizeof(int), 1, f);
    fread(p, sizeof(Player), *count, f);
    fclose(f);
    return true;
}

void RefreshSaveList()
{
    FilePathList list = LoadDirectoryFilesEx(".", ".sav", false);
    loadCount = 0;
    for (unsigned int i = 0; i < list.count && loadCount < 64; i++)
        strcpy(loadNames[loadCount++], GetFileName(list.paths[i]));
    UnloadDirectoryFiles(list);
}

int ConsoleChooseSave(char* outPath)
{
    RefreshSaveList();
    printf("\nSaved games:\n");
    if (loadCount == 0) { printf("  (none)\n"); printf("Press ENTER\n"); getchar(); return 0; }
    for (int i = 0; i < loadCount; i++) printf("  %d) %s\n", i + 1, loadNames[i]);
    printf("Choose a number: ");
    int c = 0;
    if (scanf("%d", &c) != 1) { while (getchar() != '\n'); return 0; }
    if (c<1 || c>loadCount) { while (getchar() != '\n'); return 0; }
    strcpy(outPath, loadNames[c - 1]);
    while (getchar() != '\n');
    return 1;
}

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snakes & Ladders");
    SetTargetFPS(60);
    srand((unsigned)time(NULL));
    InitBoardNumbers();
    InitSnakesAndLadders();

    GameState state = TITLE_SCREEN;

    Player players[4];
    int    playerCount = 2;
    int    currentPlayer = 0;
    int    diceResult = 1;
    float  diceTimer = 0.0f;

    Button title = createButton("C:/assets/buttons/title.png", SCREEN_WIDTH / 2 - 400, 150);
    Button background = createButton("C:/assets/buttons/background.png", SCREEN_WIDTH / 2 - 960, 0);
    Button startBtn = createButton("C:/assets/buttons/start_btn.png", SCREEN_WIDTH / 2 - 100, 690);
    Button loadBtn = createButton("C:/assets/buttons/load_btn.png", SCREEN_WIDTH / 2 - 100, 800);
    Button exitBtn = createButton("C:/assets/buttons/exit_btn.png", SCREEN_WIDTH / 2 - 100, 900);
    Button rollBtn = createButton("C:/assets/buttons/roll_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    Button throwBtn = createButton("C:/assets/buttons/throw_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    Button exitGameBtn = createButton("C:/assets/buttons/exit_btn.png", 50, 400);
    Button saveBtn = createButton("C:/assets/buttons/save_btn.png", 50, 510);
    Button twoBtn = createButton("C:/assets/buttons/2p.png", SCREEN_WIDTH / 2 - 200, 600);
    Button threeBtn = createButton("C:/assets/buttons/3p.png", SCREEN_WIDTH / 2 - 50, 600);
    Button fourBtn = createButton("C:/assets/buttons/4p.png", SCREEN_WIDTH / 2 + 100, 600);

    Texture2D dice[6];
    for (int i = 0; i < 6; i++) { char path[64]; sprintf(path, "C:/assets/dice/dice%d.png", i + 1); dice[i] = LoadTexture(path); }

    for (int i = 0; i < 4; i++)
    {
        players[i].position = 1;
        players[i].color = (Color){ rand() % 256,rand() % 256,rand() % 256,255 };
        players[i].playerNumber = i + 1;
        sprintf(players[i].name, "Player %d", i + 1);
    }

    while (!WindowShouldClose())
    {
        switch (state)
        {
        case TITLE_SCREEN:
            if (isButtonPressed(startBtn)) state = SELECT_PLAYERS;
            if (isButtonPressed(loadBtn))
            {
                char chosen[64];
                if (ConsoleChooseSave(chosen) &&
                    LoadGameFile(players, &playerCount, &currentPlayer, &diceResult, chosen))
                    state = GAME_ACTIVE;
            }
            if (isButtonPressed(exitBtn)) CloseWindow();
            break;

        case SELECT_PLAYERS:
            if (isButtonPressed(twoBtn)) { playerCount = 2; state = GAME_ACTIVE; }
            if (isButtonPressed(threeBtn)) { playerCount = 3; state = GAME_ACTIVE; }
            if (isButtonPressed(fourBtn)) { playerCount = 4; state = GAME_ACTIVE; }
            if (isButtonPressed(exitGameBtn)) state = TITLE_SCREEN;
            break;

        case GAME_ACTIVE:
            if (isButtonPressed(rollBtn)) { diceTimer = 0; state = DICE_ROLLING; }
            if (isButtonPressed(exitGameBtn)) state = TITLE_SCREEN;
            if (isButtonPressed(saveBtn)) { nameLen = 0; fileName[0] = 0; returnState = GAME_ACTIVE; state = NAME_INPUT_SAVE; }
            break;

        case DICE_ROLLING:
            diceTimer += GetFrameTime();
            if (diceTimer >= 0.12f) { diceResult = rollDice(); diceTimer = 0; }
            if (isButtonPressed(throwBtn))
            {
                stepsRemaining = diceResult; stepDir = 1; stepTimer = 0; state = PIECE_MOVING;
            }
            if (isButtonPressed(exitGameBtn)) state = TITLE_SCREEN;
            if (isButtonPressed(saveBtn)) { nameLen = 0; fileName[0] = 0; returnState = DICE_ROLLING; state = NAME_INPUT_SAVE; }
            break;

        case PIECE_MOVING:
            stepTimer += GetFrameTime();
            if (stepTimer >= STEP_DELAY)
            {
                stepTimer = 0;
                if (players[currentPlayer].position == 100) stepDir = -1;
                players[currentPlayer].position += stepDir;
                stepsRemaining--;
                if (stepsRemaining == 0)
                {
                    if (players[currentPlayer].position > 100) players[currentPlayer].position = 100 - (players[currentPlayer].position - 100);
                    players[currentPlayer].position = CheckSnakeOrLadder(players[currentPlayer].position);
                    if (strlen(fileName) > 0) SaveGameFile(players, playerCount, currentPlayer, diceResult, fileName);
                    if (players[currentPlayer].position == 100) state = GAME_OVER;
                    else { currentPlayer = (currentPlayer + 1) % playerCount; state = GAME_ACTIVE; }
                }
            }
            if (isButtonPressed(exitGameBtn)) state = TITLE_SCREEN;
            if (isButtonPressed(saveBtn)) { nameLen = 0; fileName[0] = 0; returnState = PIECE_MOVING; state = NAME_INPUT_SAVE; }
            break;

        case NAME_INPUT_SAVE:
        {
            int key = GetCharPressed();
            while (key > 0)
            {
                if (key >= 32 && key <= 126 && nameLen < 60) { fileName[nameLen++] = (char)key; fileName[nameLen] = 0; }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && nameLen > 0) { fileName[--nameLen] = 0; }
            if (IsKeyPressed(KEY_ENTER) && nameLen > 0)
            {
                strcat(fileName, ".sav");
                SaveGameFile(players, playerCount, currentPlayer, diceResult, fileName);
                state = returnState;
            }
            if (IsKeyPressed(KEY_ESCAPE)) state = returnState;
        }
        break;

        case GAME_OVER:
            if (IsKeyPressed(KEY_ENTER))
            {
                for (int i = 0; i < playerCount; i++) players[i].position = 1;
                currentPlayer = 0;
                state = TITLE_SCREEN;
            }
            break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (state)
        {
        case TITLE_SCREEN:
            DrawTexture(background.texture, background.bounds.x, background.bounds.y, WHITE);
            DrawTexture(title.texture, title.bounds.x, title.bounds.y, WHITE);
            DrawTexture(startBtn.texture, startBtn.bounds.x, startBtn.bounds.y, WHITE);
            DrawTexture(loadBtn.texture, loadBtn.bounds.x, loadBtn.bounds.y, WHITE);
            DrawTexture(exitBtn.texture, exitBtn.bounds.x, exitBtn.bounds.y, WHITE);
            break;

        case SELECT_PLAYERS:
            DrawTexture(background.texture, background.bounds.x, background.bounds.y, WHITE);
            DrawText("Select number of players", SCREEN_WIDTH / 2 - 220, 500, 40, BLACK);
            DrawTexture(twoBtn.texture, twoBtn.bounds.x, twoBtn.bounds.y, WHITE);
            DrawTexture(threeBtn.texture, threeBtn.bounds.x, threeBtn.bounds.y, WHITE);
            DrawTexture(fourBtn.texture, fourBtn.bounds.x, fourBtn.bounds.y, WHITE);
            DrawTexture(exitGameBtn.texture, exitGameBtn.bounds.x, exitGameBtn.bounds.y, WHITE);
            break;

        case GAME_ACTIVE:
            DrawBoard(); DrawSnakesAndLadders(); DrawPlayers(players, playerCount);
            DrawTexture(rollBtn.texture, rollBtn.bounds.x, rollBtn.bounds.y, WHITE);
            DrawTexture(exitGameBtn.texture, exitGameBtn.bounds.x, exitGameBtn.bounds.y, WHITE);
            DrawTexture(saveBtn.texture, saveBtn.bounds.x, saveBtn.bounds.y, WHITE);
            DrawTexture(dice[diceResult - 1], SCREEN_WIDTH - 200, 100, WHITE);
            { char info[64]; sprintf(info, "Current: %s", players[currentPlayer].name); DrawText(info, 50, 100, 30, players[currentPlayer].color); }
            break;

        case DICE_ROLLING:
            DrawBoard(); DrawSnakesAndLadders(); DrawPlayers(players, playerCount);
            DrawTexture(throwBtn.texture, throwBtn.bounds.x, throwBtn.bounds.y, WHITE);
            DrawTexture(exitGameBtn.texture, exitGameBtn.bounds.x, exitGameBtn.bounds.y, WHITE);
            DrawTexture(saveBtn.texture, saveBtn.bounds.x, saveBtn.bounds.y, WHITE);
            DrawTexture(dice[diceResult - 1], SCREEN_WIDTH - 200, 100, WHITE);
            { char info[64]; sprintf(info, "Current: %s", players[currentPlayer].name); DrawText(info, 50, 100, 30, players[currentPlayer].color); }
            break;

        case PIECE_MOVING:
            DrawBoard(); DrawSnakesAndLadders(); DrawPlayers(players, playerCount);
            DrawTexture(exitGameBtn.texture, exitGameBtn.bounds.x, exitGameBtn.bounds.y, WHITE);
            DrawTexture(saveBtn.texture, saveBtn.bounds.x, saveBtn.bounds.y, WHITE);
            DrawTexture(dice[diceResult - 1], SCREEN_WIDTH - 200, 100, WHITE);
            { char info[64]; sprintf(info, "Current: %s", players[currentPlayer].name); DrawText(info, 50, 100, 30, players[currentPlayer].color); }
            break;

        case NAME_INPUT_SAVE:
        {
            if (returnState == GAME_ACTIVE) {
                DrawBoard(); DrawSnakesAndLadders(); DrawPlayers(players, playerCount);
                DrawTexture(rollBtn.texture, rollBtn.bounds.x, rollBtn.bounds.y, WHITE);
            }
            else if (returnState == DICE_ROLLING) {
                DrawBoard(); DrawSnakesAndLadders(); DrawPlayers(players, playerCount);
                DrawTexture(throwBtn.texture, throwBtn.bounds.x, throwBtn.bounds.y, WHITE);
            }
            else if (returnState == PIECE_MOVING) {
                DrawBoard(); DrawSnakesAndLadders(); DrawPlayers(players, playerCount);
            }

            DrawTexture(exitGameBtn.texture, exitGameBtn.bounds.x, exitGameBtn.bounds.y, WHITE);
            DrawTexture(saveBtn.texture, saveBtn.bounds.x, saveBtn.bounds.y, WHITE);

            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.4f));

            int boxW = 420, boxH = 160;
            int boxX = (SCREEN_WIDTH - boxW) / 2;
            int boxY = (SCREEN_HEIGHT - boxH) / 2;
            DrawRectangle(boxX, boxY, boxW, boxH, WHITE);
            DrawRectangleLines(boxX, boxY, boxW, boxH, BLACK);

            DrawText("Save file name:", boxX + 20, boxY + 20, 28, BLACK);
            DrawRectangleLines(boxX + 20, boxY + 70, boxW - 40, 40, BLACK);
            DrawText(fileName, boxX + 30, boxY + 80, 28, BLACK);
        }
        break;

        case GAME_OVER:
            DrawText("GAME OVER", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 60, 50, RED);
            for (int i = 0; i < playerCount; i++)
                if (players[i].position == 100)
                {
                    char win[64]; sprintf(win, "Winner: %s", players[i].name); DrawText(win, SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2, 40, players[i].color);
                }
            DrawText("Press ENTER to return to title", SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 2 + 80, 28, BLACK);
            break;
        }

        EndDrawing();
    }

    for (int i = 0; i < 6; i++) UnloadTexture(dice[i]);
    CloseWindow();
    return 0;
}