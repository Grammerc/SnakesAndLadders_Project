#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ────────── constants ────────── */
#define SCREEN_WIDTH    1920
#define SCREEN_HEIGHT   1080
#define BOARD_SIZE      10
#define CELL_SIZE       80
#define BOARD_OFFSET_X  ((SCREEN_WIDTH - BOARD_SIZE * CELL_SIZE) / 2)
#define BOARD_OFFSET_Y  150
#define MAX_SNAKES      20
#define MAX_LADDERS     5

/* ────────── types ────────── */
typedef enum
{
    TITLE_SCREEN,
    SELECT_PLAYERS,
    ENTER_NAMES,
    GAME_ACTIVE,
    DICE_ROLLING,
    PIECE_MOVING,
    NAME_INPUT_SAVE,
    PLACING_SNAKE,
    GAME_OVER
} GameState;

typedef enum { MODE_CLASSIC, MODE_CHAOS } Mode;

typedef struct
{
    Texture2D texture;
    Rectangle bounds;
} Button;

typedef struct
{
    int        position;
    Color      color;
    char       name[32];
    int        playerNumber;
    Texture2D  token;
} Player;

typedef struct { int start, end; } SnakeOrLadder;

/* ────────── globals ────────── */
static GameState   state = TITLE_SCREEN;
static Mode        gMode = MODE_CLASSIC;

static Player      players[4];
static int         playerCount = 2;
static int         diceCount = 1;
static int         currentPlayer = 0;
static int         dieA = 1;
static int         dieB = 0;
static int         diceTotal = 1;

static int            boardNumbers[BOARD_SIZE][BOARD_SIZE];
static SnakeOrLadder  snakes[MAX_SNAKES];
static SnakeOrLadder  ladders[MAX_LADDERS];
static int            snakeUsed = 5;                    /* 5 preset snakes */

static int   personalTurn[4] = { 0 };
static bool  canPlace[4] = { 0 };
static int   globalTurn = 0;

/* movement */
static int   stepsRemaining = 0;
static int   stepDir = 1;
static float stepTimer = 0.0f;
static const float STEP_DELAY = 0.15f;

/* dialogs */
static char nameBuf[32] = ""; static int nameLen = 0, nameIdx = 0;
static char saveFile[64] = ""; static int saveLen = 0; static GameState returnState;
static char tileBuf[4] = ""; static int tileLen = 0;

/* load-list */
static char loadNames[64][64]; static int loadCount = 0;

/* textures */
static Texture2D diceTex[6];
static Texture2D tokenTex[4];
static Texture2D snakeSkin, ladderSkin;

/* buttons */
static Button bg, titleB, startB, loadB, exitB;
static Button twoP, threeP, fourP;
static Button oneDie, twoDice;
static Button classicBtn, chaosBtn;
static Button rollB, throwB, leaveB, saveB, placeSnakeB;

/* ────────── helper functions ────────── */
static Button makeBtn(const char* path, int x, int y)
{
    Button b;
    b.texture = LoadTexture(path);
    b.bounds = (Rectangle){ x, y, b.texture.width, b.texture.height };
    return b;
}
static bool hit(Button b)
{
    return CheckCollisionPointRec(GetMousePosition(), b.bounds) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

/* ────────── board & presets ────────── */
static void InitBoardNumbers(void)
{
    int  n = 1;
    bool right = true;

    for (int r = BOARD_SIZE - 1; r >= 0; r--)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            int col = right ? c : (BOARD_SIZE - 1 - c);
            boardNumbers[r][col] = n++;
        }
        right = !right;
    }
}
static void InitSnakesLadders(void)
{
    snakes[0] = (SnakeOrLadder){ 16,6 };  snakes[1] = (SnakeOrLadder){ 37,21 };
    snakes[2] = (SnakeOrLadder){ 49,27 }; snakes[3] = (SnakeOrLadder){ 82,55 };
    snakes[4] = (SnakeOrLadder){ 97,78 };

    ladders[0] = (SnakeOrLadder){ 4,14 }; ladders[1] = (SnakeOrLadder){ 8,30 };
    ladders[2] = (SnakeOrLadder){ 28,76 }; ladders[3] = (SnakeOrLadder){ 71,92 };
    ladders[4] = (SnakeOrLadder){ 80,99 };
}
static void ResetGame(void)
{
    playerCount = 2;
    diceCount = 1;
    currentPlayer = 0;
    dieA = dieB = diceTotal = 1;

    snakeUsed = 5;
    InitSnakesLadders();

    memset(personalTurn, 0, sizeof(personalTurn));
    memset(canPlace, 0, sizeof(canPlace));
    globalTurn = 0;
    gMode = MODE_CLASSIC;

    for (int i = 0;i < 4;i++)
    {
        players[i].position = 1;
        sprintf(players[i].name, "Player %d", i + 1);
    }
}

/* ────────── math helpers ────────── */
static Vector2 CellPos(int num)
{
    for (int r = 0;r < BOARD_SIZE;r++)
        for (int c = 0;c < BOARD_SIZE;c++)
            if (boardNumbers[r][c] == num)
                return (Vector2) {
                BOARD_OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2,
                    BOARD_OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2
            };
    return (Vector2) { 0, 0 };
}
static int Slide(int pos)
{
    for (int i = 0;i < snakeUsed;i++)
        if (snakes[i].start == pos) return snakes[i].end;

    for (int i = 0;i < MAX_LADDERS;i++)
        if (ladders[i].start == pos) return ladders[i].end;

    return pos;
}
static int RollDice(void)
{
    dieA = GetRandomValue(1, 6);

    if (diceCount == 1)
    {
        dieB = 0;
        return dieA;
    }

    dieB = GetRandomValue(1, 6);
    return dieA + dieB;
}

/* ────────── save / load ────────── */
static void SaveBinary(const char* file)
{
    FILE* fp = fopen(file, "wb"); if (!fp) return;

    fwrite(&playerCount, sizeof(int), 1, fp);
    fwrite(&currentPlayer, sizeof(int), 1, fp);
    fwrite(&diceCount, sizeof(int), 1, fp);
    fwrite(&gMode, sizeof(int), 1, fp);
    fwrite(&snakeUsed, sizeof(int), 1, fp);
    fwrite(snakes, sizeof(SnakeOrLadder), snakeUsed, fp);
    fwrite(players, sizeof(Player), playerCount, fp);

    fclose(fp);
}
static void LoadList(void)
{
    FilePathList list = LoadDirectoryFilesEx(".", ".sav", false);
    loadCount = 0;

    for (unsigned int i = 0;i < list.count && loadCount < 64;i++)
        strcpy(loadNames[loadCount++], GetFileName(list.paths[i]));

    UnloadDirectoryFiles(list);
}
static int ChooseSave(char* outName)
{
    LoadList();

    if (!loadCount)
    {
        puts("\n(no save files)\n");
        return 0;
    }

    puts("\nSave files:");
    for (int i = 0;i < loadCount;i++)
        printf("  %d) %s\n", i + 1, loadNames[i]);

    printf("Choose #: ");
    int sel = 0;
    if (scanf("%d", &sel) != 1 || sel<1 || sel>loadCount)
    {
        while (getchar() != '\n');
        return 0;
    }

    strcpy(outName, loadNames[sel - 1]);
    while (getchar() != '\n');
    return 1;
}

/* ────────── drawing ────────── */
static void DrawBoard(void)
{
    /* squares & numbers */
    for (int r = 0;r < BOARD_SIZE;r++)
    {
        for (int c = 0;c < BOARD_SIZE;c++)
        {
            Color cell = ((r + c) & 1) ? WHITE : LIGHTGRAY;

            DrawRectangle(BOARD_OFFSET_X + c * CELL_SIZE,
                BOARD_OFFSET_Y + r * CELL_SIZE,
                CELL_SIZE, CELL_SIZE, cell);

            char num[4];
            sprintf(num, "%d", boardNumbers[r][c]);

            DrawText(num,
                BOARD_OFFSET_X + c * CELL_SIZE + 8,
                BOARD_OFFSET_Y + r * CELL_SIZE + 6,
                18, DARKGRAY);

            DrawRectangleLines(BOARD_OFFSET_X + c * CELL_SIZE,
                BOARD_OFFSET_Y + r * CELL_SIZE,
                CELL_SIZE, CELL_SIZE, BLACK);
        }
    }

    /* snakes with skin */
    for (int i = 0;i < snakeUsed;i++)
    {
        Vector2 s = CellPos(snakes[i].start);
        Vector2 e = CellPos(snakes[i].end);
        Vector2 mid = { (s.x + e.x) / 2.0f, (s.y + e.y) / 2.0f };

        float dx = e.x - s.x;
        float dy = e.y - s.y;
        float length = sqrtf(dx * dx + dy * dy);
        float angle = atan2f(dy, dx) * (180.0f / PI);

        Rectangle src = { 0,0, (float)snakeSkin.width, (float)snakeSkin.height };
        Rectangle dst = { mid.x, mid.y, length, CELL_SIZE * 0.8f };
        Vector2   origin = { 0, snakeSkin.height / 2.0f };

        DrawTexturePro(snakeSkin, src, dst, origin, angle, WHITE);
    }

    /* ladders with skin */
    for (int i = 0;i < MAX_LADDERS;i++)
    {
        Vector2 s = CellPos(ladders[i].start);
        Vector2 e = CellPos(ladders[i].end);
        Vector2 mid = { (s.x + e.x) / 2.0f, (s.y + e.y) / 2.0f };

        float dx = e.x - s.x;
        float dy = e.y - s.y;
        float length = sqrtf(dx * dx + dy * dy);
        float angle = atan2f(dy, dx) * (180.0f / PI);

        Rectangle src = { 0,0, (float)ladderSkin.width, (float)ladderSkin.height };
        Rectangle dst = { mid.x, mid.y, length, CELL_SIZE * 0.8f };
        Vector2   origin = { 0, ladderSkin.height / 2.0f };

        DrawTexturePro(ladderSkin, src, dst, origin, angle, WHITE);
    }
}
static void DrawPlayers(void)
{
    for (int i = 0;i < playerCount;i++)
    {
        Vector2 p = CellPos(players[i].position);
        float   r = CELL_SIZE / 4.0f;

        p.x += cosf(2 * PI * i / playerCount) * r * 0.6f;
        p.y += sinf(2 * PI * i / playerCount) * r * 0.6f;

        DrawTexture(players[i].token,
            p.x - players[i].token.width / 2,
            p.y - players[i].token.height / 2,
            WHITE);
    }
}

/* ────────── asset loading ────────── */
static void LoadAssets(void)
{
    bg = makeBtn("assets/buttons/background.png", SCREEN_WIDTH / 2 - 960, 0);
    titleB = makeBtn("assets/buttons/title.png", SCREEN_WIDTH / 2 - 400, 140);
    startB = makeBtn("assets/buttons/start_btn.png", SCREEN_WIDTH / 2 - 140, 620);
    loadB = makeBtn("assets/buttons/load_btn.png", SCREEN_WIDTH / 2 - 140, 700);
    exitB = makeBtn("assets/buttons/exit_btn.png", SCREEN_WIDTH / 2 - 140, 780);

    twoP = makeBtn("assets/buttons/2p.png", SCREEN_WIDTH / 2 - 340, 420);
    threeP = makeBtn("assets/buttons/3p.png", SCREEN_WIDTH / 2 - 70, 420);
    fourP = makeBtn("assets/buttons/4p.png", SCREEN_WIDTH / 2 + 200, 420);

    oneDie = makeBtn("assets/buttons/one_die_btn.png", SCREEN_WIDTH / 2 - 280, 540);
    twoDice = makeBtn("assets/buttons/two_dice_btn.png", SCREEN_WIDTH / 2 + 40, 540);

    classicBtn = makeBtn("assets/buttons/Classic_Mode_btn.png", SCREEN_WIDTH / 2 - 380, 660);
    chaosBtn = makeBtn("assets/buttons/Chaos_Mode_btn.png", SCREEN_WIDTH / 2 + 80, 660);

    rollB = makeBtn("assets/buttons/roll_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    throwB = makeBtn("assets/buttons/throw_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    leaveB = makeBtn("assets/buttons/exit_btn.png", 50, 400);
    saveB = makeBtn("assets/buttons/save_btn.png", 50, 520);
    placeSnakeB = makeBtn("assets/buttons/place_snake_btn.png", SCREEN_WIDTH - 320, 260);

    for (int i = 0;i < 6;i++)
    {
        char p[64]; sprintf(p, "assets/dice/dice%d.png", i + 1);
        diceTex[i] = LoadTexture(p);
    }
    for (int i = 0;i < 4;i++)
    {
        char p[64]; sprintf(p, "assets/tokens/token%d.png", i + 1);
        tokenTex[i] = LoadTexture(p);
    }

    snakeSkin = LoadTexture("assets/skins/snake.png");
    ladderSkin = LoadTexture("assets/skins/ladder.png");
}

/* ────────── main ────────── */
int main(void)
{
    /* ---------- init ---------- */
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snakes & Ladders");
    SetTargetFPS(60);

    srand((unsigned)time(NULL));

    InitBoardNumbers();
    InitSnakesLadders();
    LoadAssets();
    ResetGame();

    for (int i = 0; i < 4; i++)               /* assign default token textures */
        players[i].token = tokenTex[i];

    /* ---------- game loop ---------- */
    while (!WindowShouldClose())
    {
        /* ==================== update / logic ==================== */
        switch (state)
        {
            /* ---------- TITLE ---------- */
        case TITLE_SCREEN:
            if (hit(startB)) state = SELECT_PLAYERS;

            if (hit(loadB))
            {
                char path[64];
                if (ChooseSave(path))
                {
                    FILE* fp = fopen(path, "rb");
                    if (fp)
                    {
                        fread(&playerCount, sizeof(int), 1, fp);
                        fread(&currentPlayer, sizeof(int), 1, fp);
                        fread(&diceCount, sizeof(int), 1, fp);
                        fread(&gMode, sizeof(int), 1, fp);
                        fread(&snakeUsed, sizeof(int), 1, fp);
                        fread(snakes, sizeof(SnakeOrLadder), snakeUsed, fp);
                        fread(players, sizeof(Player), playerCount, fp);
                        fclose(fp);
                        state = GAME_ACTIVE;
                    }
                }
            }

            if (hit(exitB)) CloseWindow();
            break;

            /* ---------- SETUP ---------- */
        case SELECT_PLAYERS:
            if (hit(twoP))     playerCount = 2;
            if (hit(threeP))   playerCount = 3;
            if (hit(fourP))    playerCount = 4;

            if (hit(oneDie))   diceCount = 1;
            if (hit(twoDice))  diceCount = 2;

            if (hit(classicBtn)) gMode = MODE_CLASSIC;
            if (hit(chaosBtn))   gMode = MODE_CHAOS;

            if (hit(startB))
            {
                state = ENTER_NAMES;
                nameIdx = 0;
                nameLen = 0;
                nameBuf[0] = '\0';
            }

            if (hit(leaveB))
            {
                ResetGame();
                state = TITLE_SCREEN;
            }
            break;

            /* ---------- NAME ENTRY ---------- */
        case ENTER_NAMES:
        {
            int ch = GetCharPressed();
            while (ch > 0)
            {
                if (ch >= 32 && ch <= 126 && nameLen < 31)
                {
                    nameBuf[nameLen++] = (char)ch;
                    nameBuf[nameLen] = '\0';
                }
                ch = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && nameLen > 0)
                nameBuf[--nameLen] = '\0';

            if (IsKeyPressed(KEY_ENTER) && nameLen > 0)
            {
                strncpy(players[nameIdx].name, nameBuf, 31);
                players[nameIdx].name[31] = '\0';

                nameIdx++;
                nameLen = 0;
                nameBuf[0] = '\0';

                if (nameIdx == playerCount) state = GAME_ACTIVE;
            }

            if (IsKeyPressed(KEY_ESCAPE))
            {
                ResetGame();
                state = TITLE_SCREEN;
            }
        }
        break;

        /* ---------- IN-GAME ---------- */
        case GAME_ACTIVE:
            if (hit(rollB))
            {
                diceTotal = RollDice();
                state = DICE_ROLLING;
            }

            if (hit(saveB))
            {
                saveLen = 0;
                saveFile[0] = '\0';
                returnState = GAME_ACTIVE;
                state = NAME_INPUT_SAVE;
            }

            if (hit(leaveB))
            {
                ResetGame();
                state = TITLE_SCREEN;
            }

            if (gMode == MODE_CHAOS && canPlace[currentPlayer] && hit(placeSnakeB))
            {
                tileLen = 0;
                tileBuf[0] = '\0';
                state = PLACING_SNAKE;
            }
            break;

            /* ---------- DICE ROLL ---------- */
        case DICE_ROLLING:
            if (hit(throwB))
            {
                stepsRemaining = diceTotal;
                stepDir = 1;
                stepTimer = 0.0f;
                state = PIECE_MOVING;
            }

            if (hit(saveB))
            {
                saveLen = 0;
                saveFile[0] = '\0';
                returnState = DICE_ROLLING;
                state = NAME_INPUT_SAVE;
            }

            if (hit(leaveB))
            {
                ResetGame();
                state = TITLE_SCREEN;
            }
            break;

            /* ---------- PIECE MOVEMENT ---------- */
        case PIECE_MOVING:
            stepTimer += GetFrameTime();

            if (stepTimer >= STEP_DELAY)
            {
                stepTimer = 0.0f;
                players[currentPlayer].position += stepDir;
                stepsRemaining--;

                if (stepsRemaining == 0)
                {
                    if (players[currentPlayer].position > 100)
                        players[currentPlayer].position =
                        100 - (players[currentPlayer].position - 100);

                    players[currentPlayer].position =
                        Slide(players[currentPlayer].position);

                    personalTurn[currentPlayer]++;
                    globalTurn++;

                    if (gMode == MODE_CHAOS && personalTurn[currentPlayer] == 2)
                    {
                        personalTurn[currentPlayer] = 0;
                        if (snakeUsed < MAX_SNAKES)
                            canPlace[currentPlayer] = true;
                    }

                    if (players[currentPlayer].position == 100)
                        state = GAME_OVER;
                    else
                    {
                        currentPlayer = (currentPlayer + 1) % playerCount;
                        state = GAME_ACTIVE;
                    }
                }
            }

            if (hit(saveB))
            {
                saveLen = 0;
                saveFile[0] = '\0';
                returnState = PIECE_MOVING;
                state = NAME_INPUT_SAVE;
            }

            if (hit(leaveB))
            {
                ResetGame();
                state = TITLE_SCREEN;
            }
            break;

            /* ---------- CHAOS: PLACE SNAKE ---------- */
        case PLACING_SNAKE:
        {
            int ch = GetCharPressed();
            while (ch > 0)
            {
                if (ch >= '0' && ch <= '9' && tileLen < 3)
                {
                    tileBuf[tileLen++] = (char)ch;
                    tileBuf[tileLen] = '\0';
                }
                ch = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && tileLen > 0)
                tileBuf[--tileLen] = '\0';

            if (IsKeyPressed(KEY_ENTER) && tileLen > 0)
            {
                int head = atoi(tileBuf);

                if (head > 1 && head < 100 && snakeUsed < MAX_SNAKES)
                {
                    int tail = head - GetRandomValue(5, 20);
                    if (tail < 1) tail = 1;

                    snakes[snakeUsed++] = (SnakeOrLadder){ head, tail };
                    canPlace[currentPlayer] = false;
                    state = GAME_ACTIVE;
                }
            }

            if (IsKeyPressed(KEY_ESCAPE))
            {
                canPlace[currentPlayer] = false;
                state = GAME_ACTIVE;
            }
        }
        break;

        /* ---------- SAVE DIALOG ---------- */
        case NAME_INPUT_SAVE:
        {
            int ch = GetCharPressed();
            while (ch > 0)
            {
                if (ch >= 32 && ch <= 126 && saveLen < 60)
                {
                    saveFile[saveLen++] = (char)ch;
                    saveFile[saveLen] = '\0';
                }
                ch = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && saveLen > 0)
                saveFile[--saveLen] = '\0';

            if (IsKeyPressed(KEY_ENTER) && saveLen > 0)
            {
                strcat(saveFile, ".sav");
                SaveBinary(saveFile);
                state = returnState;
            }

            if (IsKeyPressed(KEY_ESCAPE))
                state = returnState;
        }
        break;

        /* ---------- GAME OVER ---------- */
        case GAME_OVER:
            if (IsKeyPressed(KEY_ENTER))
            {
                ResetGame();
                state = TITLE_SCREEN;
            }
            break;
        }

        /* ==================== draw ==================== */
        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (state)
        {
            /* ---------- TITLE ---------- */
        case TITLE_SCREEN:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            DrawTexture(titleB.texture, titleB.bounds.x, titleB.bounds.y, WHITE);
            DrawTexture(startB.texture, startB.bounds.x, startB.bounds.y, WHITE);
            DrawTexture(loadB.texture, loadB.bounds.x, loadB.bounds.y, WHITE);
            DrawTexture(exitB.texture, exitB.bounds.x, exitB.bounds.y, WHITE);
            break;

            /* ---------- SETUP ---------- */
        case SELECT_PLAYERS:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);

            DrawText("Setup Game", SCREEN_WIDTH / 2 - 160, 240, 60, BLACK);

            DrawText("Players", SCREEN_WIDTH / 2 - 100, 370, 38, DARKBLUE);
            DrawText("Dice", SCREEN_WIDTH / 2 - 58, 490, 38, DARKBLUE);
            DrawText("Mode", SCREEN_WIDTH / 2 - 62, 610, 38, DARKBLUE);

            DrawTexture(twoP.texture, twoP.bounds.x, twoP.bounds.y, WHITE);
            DrawTexture(threeP.texture, threeP.bounds.x, threeP.bounds.y, WHITE);
            DrawTexture(fourP.texture, fourP.bounds.x, fourP.bounds.y, WHITE);

            DrawTexture(oneDie.texture, oneDie.bounds.x, oneDie.bounds.y, WHITE);
            DrawTexture(twoDice.texture, twoDice.bounds.x, twoDice.bounds.y, WHITE);

            DrawTexture(classicBtn.texture, classicBtn.bounds.x, classicBtn.bounds.y, WHITE);
            DrawTexture(chaosBtn.texture, chaosBtn.bounds.x, chaosBtn.bounds.y, WHITE);

            DrawTexture(startB.texture, startB.bounds.x, startB.bounds.y, WHITE);

            DrawRectangleLinesEx(twoP.bounds, 3, (playerCount == 2) ? RED : BLACK);
            DrawRectangleLinesEx(threeP.bounds, 3, (playerCount == 3) ? RED : BLACK);
            DrawRectangleLinesEx(fourP.bounds, 3, (playerCount == 4) ? RED : BLACK);
            DrawRectangleLinesEx(oneDie.bounds, 3, (diceCount == 1) ? RED : BLACK);
            DrawRectangleLinesEx(twoDice.bounds, 3, (diceCount == 2) ? RED : BLACK);
            DrawRectangleLinesEx(classicBtn.bounds, 4, (gMode == MODE_CLASSIC) ? GREEN : BLACK);
            DrawRectangleLinesEx(chaosBtn.bounds, 4, (gMode == MODE_CHAOS) ? GREEN : BLACK);

            DrawTexture(leaveB.texture, leaveB.bounds.x, leaveB.bounds.y, WHITE);
            break;

            /* ---------- ENTER NAME ---------- */
        case ENTER_NAMES:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);

            DrawText(TextFormat("Enter name for Player %d", nameIdx + 1),
                SCREEN_WIDTH / 2 - 280, 380, 48, BLACK);

            DrawRectangleLines(SCREEN_WIDTH / 2 - 300, 460, 600, 70, BLACK);
            DrawText(nameBuf, SCREEN_WIDTH / 2 - 290, 475, 40, BLACK);
            break;

            /* ---------- MAIN GAME STATES ---------- */
        case GAME_ACTIVE:
        case DICE_ROLLING:
        case PIECE_MOVING:
        case PLACING_SNAKE:
        case NAME_INPUT_SAVE:
            DrawBoard();
            DrawPlayers();
            break;

        default: break;
        }

        /* ---------- overlay UI for main-game states ---------- */
        if (state == GAME_ACTIVE || state == DICE_ROLLING || state == PIECE_MOVING)
        {
            DrawTexture(rollB.texture, rollB.bounds.x, rollB.bounds.y, WHITE);
            if (state != GAME_ACTIVE)                      /* rolling or moving */
                DrawTexture(throwB.texture, throwB.bounds.x, throwB.bounds.y, WHITE);

            DrawTexture(leaveB.texture, leaveB.bounds.x, leaveB.bounds.y, WHITE);
            DrawTexture(saveB.texture, saveB.bounds.x, saveB.bounds.y, WHITE);

            if (gMode == MODE_CHAOS && canPlace[currentPlayer])
                DrawTexture(placeSnakeB.texture, placeSnakeB.bounds.x, placeSnakeB.bounds.y, WHITE);

            /* dice faces */
            if (diceCount == 1)
                DrawTexture(diceTex[dieA - 1], SCREEN_WIDTH - 200, 100, WHITE);
            else
            {
                DrawTexture(diceTex[dieA - 1], SCREEN_WIDTH - 240, 100, WHITE);
                DrawTexture(diceTex[dieB - 1], SCREEN_WIDTH - 160, 100, WHITE);
            }

            DrawText(TextFormat("Current Turn: %s", players[currentPlayer].name),
                50, 100, 32, players[currentPlayer].color);

            DrawText(TextFormat("Global Turn: %d", globalTurn),
                SCREEN_WIDTH - 260, 40, 28, BLACK);
        }

        /* ---------- special dialogs ---------- */
        if (state == NAME_INPUT_SAVE)
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));

            DrawRectangle(SCREEN_WIDTH / 2 - 320, 380, 640, 180, WHITE);
            DrawRectangleLines(SCREEN_WIDTH / 2 - 320, 380, 640, 180, BLACK);

            DrawText("Save file name:", SCREEN_WIDTH / 2 - 300, 400, 32, BLACK);
            DrawRectangleLines(SCREEN_WIDTH / 2 - 300, 450, 600, 50, BLACK);
            DrawText(saveFile, SCREEN_WIDTH / 2 - 290, 460, 30, BLACK);
        }
        else if (state == PLACING_SNAKE)
        {
            DrawTexture(placeSnakeB.texture, placeSnakeB.bounds.x, placeSnakeB.bounds.y, WHITE);

            DrawRectangle(SCREEN_WIDTH - 320, 120, 300, 140, WHITE);
            DrawRectangleLines(SCREEN_WIDTH - 320, 120, 300, 140, BLACK);
            DrawText("Head tile (2-99):", SCREEN_WIDTH - 300, 135, 22, BLACK);
            DrawRectangleLines(SCREEN_WIDTH - 300, 170, 260, 40, BLACK);
            DrawText(tileBuf, SCREEN_WIDTH - 290, 178, 28, BLACK);
        }
        else if (state == GAME_OVER)
        {
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            DrawText("GAME OVER", SCREEN_WIDTH / 2 - 180, 360, 60, RED);
            DrawText("Press ENTER", SCREEN_WIDTH / 2 - 140, 440, 38, BLACK);
        }

        EndDrawing();
    }

    /* ---------- unload ---------- */
    for (int i = 0;i < 6;i++) UnloadTexture(diceTex[i]);
    for (int i = 0;i < 4;i++) UnloadTexture(tokenTex[i]);
    UnloadTexture(snakeSkin);
    UnloadTexture(ladderSkin);

    CloseWindow();
    return 0;
}