#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SCREEN_WIDTH    1920
#define SCREEN_HEIGHT   1080
#define BOARD_SIZE      10
#define CELL_SIZE       80
#define BOARD_OFFSET_X  ((SCREEN_WIDTH - BOARD_SIZE * CELL_SIZE) / 2) //center screen 
#define BOARD_OFFSET_Y  150
#define MAX_SNAKES      20
#define MAX_LADDERS     5

typedef enum {
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

typedef enum { 
    MODE_CLASSIC,
    MODE_CHAOS
} Mode;

typedef struct {
    Texture2D texture;
    Rectangle bounds;
} Button;

typedef struct {
    int position;
    Color color;
    char name[32];
    int playerNumber;
    Texture2D  token;
} Player;

typedef struct {
    int   position;
    Color color;
    char  name[32];
    int   playerNumber;
} SavePlayer;

typedef struct SnakeOrLadder {
    int start;
    int end;
    struct SnakeOrLadder* next;
} SnakeOrLadder;


static GameState state = TITLE_SCREEN;
static Mode gMode = MODE_CLASSIC;

static Player players[4];
static int playerCount = 2;
static int diceCount = 1;
static int currentPlayer = 0;        
static int dieA = 1, dieB = 0;      
static int diceTotal = 1;       

static int boardNumbers[BOARD_SIZE][BOARD_SIZE];
static SnakeOrLadder* snakes = NULL;
static SnakeOrLadder* ladders = NULL;
static int snakeCount = 0;

static int personalTurn[4] = { 0 };
static bool canPlace[4] = { 0 };
static int globalTurn = 0;

static bool diceAnimating = false;
static int animFaceA = 1;
static int animFaceB = 1;
static float diceAnimTimer = 0.f;
static const float DICE_FRAME = 0.08f;  

static int stepsRemaining = 0;
static int stepDir = 1;    
static float stepTimer = 0.f;
static const float STEP_DELAY = 0.15f;
static bool bouncing = false;  

static int winnerIdx = -1;

static char nameBuf[32] = ""; 
static int nameLen = 0, nameIdx = 0;
static char saveFile[64] = ""; 
static int saveLen = 0;
static GameState returnState;
static char tileBuf[4] = ""; 
static int tileLen = 0;

static char loadNames[64][64];
static int loadCount = 0;

static Texture2D diceTex[6];
static Texture2D tokenTex[4];

static Button bg, titleB, startB, loadB, exitB;
static Button twoP, threeP, fourP, oneDie, twoDice, classicBtn, chaosBtn;
static Button playB;
static Button rollB, throwB, leaveB, saveB, placeSnakeB;

static Button makeBtn(const char* path, int x, int y)
{
    Button b;
    b.texture = LoadTexture(path);
    b.bounds = (Rectangle){
        x, y,
        b.texture.width, 
        b.texture.height 
    };
    return b;
}
static bool hit(Button b)
{
    return CheckCollisionPointRec(GetMousePosition(), b.bounds) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static inline int DiceFaceA(void)
{
    return (state == DICE_ROLLING) ? animFaceA : dieA;
}
static inline int DiceFaceB(void)
{
    if (diceCount == 1) {
        return 0;
    }
    return (state == DICE_ROLLING) ? animFaceB : dieB;
}

static void FreeSnakesAndLadders(void)
{
    SnakeOrLadder* n = snakes;
    while (n){ SnakeOrLadder * nxt = n->next;
    free(n);
    n = nxt; }
    n = ladders;

    while (n){ 
        SnakeOrLadder * nxt = n->next;
        free(n);
        n = nxt; }
    snakes = ladders = NULL;
    snakeCount = 0;
}

static void InitBoardNumbers(void)
{
    int n = 1;
    bool right = true;
    for (int r = BOARD_SIZE - 1; r >= 0; --r)
    {
        for (int c = 0; c < BOARD_SIZE; ++c)
            boardNumbers[r][right ? c : (BOARD_SIZE - 1 - c)] = n++;
        right = !right;
    }
}

static void PushLink(SnakeOrLadder** head, int s, int e)
{
    //linked list implementation
    SnakeOrLadder* n = (SnakeOrLadder*)malloc(sizeof(SnakeOrLadder));
    n->start = s;
    n->end = e;
    n->next = *head;
    *head = n;
}
static void InitSnakesLadders(void)
{
    FreeSnakesAndLadders();
 
    PushLink(&snakes, 97, 78);
    PushLink(&snakes, 82, 55);
    PushLink(&snakes, 49, 27);
    PushLink(&snakes, 37, 21);
    PushLink(&snakes, 16, 6);
    snakeCount = 5; //default 5 to check for placed snakes
 
    PushLink(&ladders, 80, 99);
    PushLink(&ladders, 71, 92);
    PushLink(&ladders, 28, 76);
    PushLink(&ladders, 8, 30);
    PushLink(&ladders, 4, 14);
}

static void ResetGame(void)
{
    playerCount = 2; diceCount = 1; currentPlayer = 0;
    dieA = dieB = diceTotal = 1;
    InitSnakesLadders();
    memset(personalTurn, 0, sizeof(personalTurn));
    memset(canPlace, 0, sizeof(canPlace));
    globalTurn = 0;  gMode = MODE_CLASSIC;
    bouncing = false; winnerIdx = -1;

    for (int i = 0; i < 4; ++i) {
        players[i].position = 1;
        players[i].color = (Color){
            GetRandomValue(50, 255),
            GetRandomValue(50, 255),
            GetRandomValue(50, 255), 255
        };
        sprintf(players[i].name, "Player %d", i + 1);
        players[i].playerNumber = i + 1;
        players[i].token = tokenTex[i];
    }
}

static Vector2 CellPos(int num)
{
    if (num < 1 || num>100) return (Vector2) {
        -500, -500 
    };
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (boardNumbers[r][c] == num)
                return (Vector2) {
                BOARD_OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2,
                    BOARD_OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2
            };
    return (Vector2) { 0, 0 }; 
}
static int Slide(int pos)
{
    for (SnakeOrLadder* s = snakes; s; s = s->next) {
        if (s->start == pos) {
            return s->end;
        }
    }
    for (SnakeOrLadder* l = ladders; l; l = l->next) {
        if (l->start == pos) {
            return l->end;
        }
    }
    return pos;
}
static int RollDice(void)
{
    dieA = GetRandomValue(1, 6);
    if (diceCount == 1) { dieB = 0; return dieA; }
    dieB = GetRandomValue(1, 6);
    return dieA + dieB;
}

static bool TileOccupied(int tile)
{
    for (SnakeOrLadder* s = snakes; s; s = s->next)
        if (s->start == tile || s->end == tile)
            return true;

    for (SnakeOrLadder* l = ladders; l; l = l->next)
        if (l->start == tile || l->end == tile)
            return true;

    return false;
}

static void SaveBinary(const char* fn)
{
    FILE* fp = fopen(fn, "wb");
    if (!fp) {
        perror("save");
        return;
    }


    fwrite(&playerCount, sizeof(int), 1, fp);
    fwrite(&currentPlayer, sizeof(int), 1, fp);
    fwrite(&diceCount, sizeof(int), 1, fp);
    fwrite(&gMode, sizeof(int), 1, fp);


    int safeSnakeCount = snakeCount;
    if (safeSnakeCount < 0) {
        safeSnakeCount = 0;
    }
    if (safeSnakeCount > MAX_SNAKES) {
        safeSnakeCount = MAX_SNAKES;
    }
    fwrite(&safeSnakeCount, sizeof(int), 1, fp);

    SnakeOrLadder* current = snakes;
    for (int i = 0; i < safeSnakeCount; i++)
    {
        fwrite(&current->start, sizeof(int), 1, fp);
        fwrite(&current->end, sizeof(int), 1, fp);
        current = current->next;
    }

    SavePlayer buf[4];
    for (int i = 0; i < playerCount; ++i)
    {
        buf[i].position = players[i].position;
        buf[i].color = players[i].color;
        strcpy(buf[i].name, players[i].name);
        buf[i].playerNumber = players[i].playerNumber;
    }
    fwrite(buf, sizeof(SavePlayer),
        (size_t)playerCount, fp);

    fclose(fp);
}

static int LoadBinary(const char* fn)
{
    FILE* fp = fopen(fn, "rb");
    if (!fp) { 
        perror("load"); 
    return 0; }

    if (fread(&playerCount, sizeof(int), 1, fp) != 1) {
        goto bad;
    }
    fread(&currentPlayer, sizeof(int), 1, fp);
    fread(&diceCount, sizeof(int), 1, fp);
    fread(&gMode, sizeof(int), 1, fp);

    int fileSnakeCount = 0;
    fread(&fileSnakeCount, sizeof(int), 1, fp);
    if (fileSnakeCount < 0 || fileSnakeCount > MAX_SNAKES) {
        goto bad;
    }
    snakeCount = fileSnakeCount;      

    snakes = (SnakeOrLadder*)malloc(sizeof(SnakeOrLadder));
    SnakeOrLadder* current = snakes;
    for (int i = 0; i < snakeCount; i++)
    {
        if (fread(&current->start, sizeof(int), 1, fp) != 1) goto bad;
        if (fread(&current->end, sizeof(int), 1, fp) != 1) goto bad;
        if (i < snakeCount - 1)
        {
            current->next = (SnakeOrLadder*)malloc(sizeof(SnakeOrLadder));
            current = current->next;
        }
        else
        {
            current->next = NULL;
        }
    }

    if (playerCount < 1 || playerCount > 4) goto bad;

    SavePlayer buf[4];
    if (fread(buf, sizeof(SavePlayer), (size_t)playerCount, fp) != (size_t)playerCount) {
        goto bad;
    }

    fclose(fp);

    for (int i = 0; i < playerCount; ++i)
    {
        players[i].position = buf[i].position;
        players[i].color = buf[i].color;
        strcpy(players[i].name, buf[i].name);
        players[i].playerNumber = buf[i].playerNumber;
        players[i].token = tokenTex[i];
    }
    return 1;

bad:
    fclose(fp);
    fprintf(stderr, "Something went wrong error message: %s\n", fn);
    return 0;
}

static void LoadList(void)
{
    FilePathList list = LoadDirectoryFilesEx(".", ".sav", false);
    loadCount = 0;

    for (unsigned int i = 0; i < list.count && loadCount < 64; i++)
        strcpy(loadNames[loadCount++], list.paths[i]);   /* full path */

    UnloadDirectoryFiles(list);
}

static int ChooseSave(char* out)
{
    LoadList();
    if (!loadCount) { puts("(no save files)"); return 0; }

    puts("\nSave files:");
    for (int i = 0; i < loadCount; i++)
        printf("  %d) %s\n", i + 1, GetFileName(loadNames[i]));

    printf("Choose #: ");
    int sel = 0;
    if (scanf("%d", &sel) != 1 || sel < 1 || sel > loadCount)
    {
        while (getchar() != '\n'); return 0;
    }

    strcpy(out, loadNames[sel - 1]);
    while (getchar() != '\n');
    return 1;
}


static void DrawBoard(void)
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            Color cell = ((r + c) & 1) ? WHITE : LIGHTGRAY;
            DrawRectangle(BOARD_OFFSET_X + c * CELL_SIZE,
                BOARD_OFFSET_Y + r * CELL_SIZE,
                CELL_SIZE, CELL_SIZE, cell);
            char t[4]; sprintf(t, "%d", boardNumbers[r][c]);
            DrawText(t, BOARD_OFFSET_X + c * CELL_SIZE + 8,
                BOARD_OFFSET_Y + r * CELL_SIZE + 6,
                18, DARKGRAY);
            DrawRectangleLines(BOARD_OFFSET_X + c * CELL_SIZE,
                BOARD_OFFSET_Y + r * CELL_SIZE,
                CELL_SIZE, CELL_SIZE, BLACK);
        }

    for (SnakeOrLadder* s = snakes; s; s = s->next)
        DrawLineEx(CellPos(s->start), CellPos(s->end), 5, RED);

    for (SnakeOrLadder* l = ladders; l; l = l->next)
        DrawLineEx(CellPos(l->start), CellPos(l->end), 5, GREEN);
}
static void DrawPlayers(void)
{
    for (int i = 0; i < playerCount; i++)
    {
        /* 2*PI*i/player_count 
           *0.6f <<<--- to center them
        
        */

        Vector2 p = CellPos(players[i].position); //gets pixel coordinates
        float r = CELL_SIZE / 4.f; // radial distance to make sure it does not go outside bcz 80/4 is 20 and 4 players max so wow
        p.x += cosf(2 * PI * i / playerCount) * r * 0.6f; //logic from before token images for cell position
        p.y += sinf(2 * PI * i / playerCount) * r * 0.6f; 
        DrawTexture(players[i].token, p.x - players[i].token.width / 2, p.y - players[i].token.height / 2, WHITE);
    }
}

static void LoadAssets(void)
{
    bg = makeBtn("assets/buttons/background.png", SCREEN_WIDTH / 2 - 960, 0);
    titleB = makeBtn("assets/buttons/title.png", SCREEN_WIDTH / 2 - 400, 140);
    startB = makeBtn("assets/buttons/start_btn.png", SCREEN_WIDTH / 2 - 140, 620);
    loadB = makeBtn("assets/buttons/load_btn.png", SCREEN_WIDTH / 2 - 140, 725);
    exitB = makeBtn("assets/buttons/exit_btn.png", SCREEN_WIDTH / 2 - 140, 825);

    twoP = makeBtn("assets/buttons/2p.png", 1062, 304);
    threeP = makeBtn("assets/buttons/3p.png", 1360, 304);
    fourP = makeBtn("assets/buttons/4p.png", 1657, 304);

    oneDie = makeBtn("assets/buttons/one_die_btn.png", 1064, 468);
    twoDice = makeBtn("assets/buttons/two_dice_btn.png", 1362, 468);

    classicBtn = makeBtn("assets/buttons/Classic_Mode_btn.png", 1064, 636);
    chaosBtn = makeBtn("assets/buttons/Chaos_Mode_btn.png", 1361, 636);

    playB = makeBtn("assets/buttons/play_btn.png", SCREEN_WIDTH / 2 - 120, 760);

    rollB = makeBtn("assets/buttons/roll_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    throwB = makeBtn("assets/buttons/throw_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    leaveB = makeBtn("assets/buttons/exit_btn.png", 50, 400);
    saveB = makeBtn("assets/buttons/save_btn.png", 50, 520);
    placeSnakeB = makeBtn("assets/buttons/place_snake_btn.png", SCREEN_WIDTH - 320, 260);

    for (int i = 0; i < 6; i++) { 
        char p[64];
        sprintf(p, "assets/dice/dice%d.png", i + 1);
        diceTex[i] = LoadTexture(p);
    }
    for (int i = 0; i < 4; i++) {
        char p[64];
        sprintf(p, "assets/tokens/token%d.png", i + 1);
        tokenTex[i] = LoadTexture(p);
    }
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snakes & Ladders");
    SetTargetFPS(60);

    srand((unsigned)time(NULL));
    InitBoardNumbers();
    InitSnakesLadders();
    LoadAssets();
    ResetGame();
    for (int i = 0; i < 4; i++) players[i].token = tokenTex[i];

    while (!WindowShouldClose())
    {
        switch (state)
        {
        case TITLE_SCREEN:
            if (hit(startB)) state = SELECT_PLAYERS;
            if (hit(loadB)) {
                char p[64];
                if (ChooseSave(p) && LoadBinary(p)) state = GAME_ACTIVE; 
            }
            if (hit(exitB)) { 
                CloseWindow(); 
                exit(0); 
            }
            break;

        case SELECT_PLAYERS:
            if (hit(twoP))   playerCount = 2;
            if (hit(threeP)) playerCount = 3;
            if (hit(fourP))  playerCount = 4;

            if (hit(oneDie))  diceCount = 1;
            if (hit(twoDice)) diceCount = 2;

            if (hit(classicBtn)) gMode = MODE_CLASSIC;
            if (hit(chaosBtn))   gMode = MODE_CHAOS;

            if (hit(playB)) { 
                state = ENTER_NAMES; 
                nameIdx = 0; 
                nameLen = 0; 
                nameBuf[0] = '\0';
            }
            if (hit(leaveB)) {
                ResetGame();
                state = TITLE_SCREEN; 
            }
            break;

        case ENTER_NAMES:
        {
            int ch = GetCharPressed();
            while (ch > 0) {
                if (ch >= 32 && ch <= 126 && nameLen < 31) {
                    nameBuf[nameLen++] = (char)ch;
                    nameBuf[nameLen] = '\0'; 
                }
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && nameLen > 0) {
                nameBuf[--nameLen] = '\0';
            }
            if (IsKeyPressed(KEY_ENTER) && nameLen > 0) {
                strncpy(players[nameIdx].name, nameBuf, 31);
                players[nameIdx].name[31] = '\0';
                nameIdx++; nameLen = 0; nameBuf[0] = '\0';
                if (nameIdx == playerCount) {
                    state = GAME_ACTIVE;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                ResetGame(); 
                state = TITLE_SCREEN;
            }
        } break;

        case GAME_ACTIVE:
            if (hit(rollB)) {
                diceTotal = RollDice();      
                diceAnimating = true;
                    diceAnimTimer = 0.f;
                animFaceA = 1; animFaceB = 1;
                state = DICE_ROLLING;
            }

            if (gMode == MODE_CHAOS && canPlace[currentPlayer] && hit(placeSnakeB))
            {
                tileLen = 0; tileBuf[0] = '\0'; state = PLACING_SNAKE;
            }

            if (hit(saveB)) { saveLen = 0;
            saveFile[0] = '\0';
            returnState = GAME_ACTIVE;
            state = NAME_INPUT_SAVE;
            }
            if (hit(leaveB)) { 
                ResetGame(); state = TITLE_SCREEN; 
            }
            if (hit(exitB)) {
                CloseWindow();
                return 0;
            }
            break;

        case DICE_ROLLING:
            if (diceAnimating) {
                diceAnimTimer += GetFrameTime();
                if (diceAnimTimer >= DICE_FRAME) {
                    diceAnimTimer -= DICE_FRAME;
                    animFaceA = (animFaceA % 6) + 1;
                    if (diceCount == 2) {
                        animFaceB = (animFaceB % 6) + 1; 
                    }
                }
            }
            if (hit(throwB)) {
                diceAnimating = false;
                animFaceA = dieA;
                if (diceCount == 2) {
                    animFaceB = dieB;
                }
                stepsRemaining = diceTotal;
                stepDir = 1;
                stepTimer = 0.f;
                bouncing = false;
                state = PIECE_MOVING;
            }
            if (hit(saveB)) {
                saveLen = 0; 
                saveFile[0] = '\0'; 
                returnState = DICE_ROLLING;
                state = NAME_INPUT_SAVE; 
            }
            if (hit(leaveB)) {
                ResetGame(); 
                state = TITLE_SCREEN; 
            }
            if (hit(exitB)) {
                CloseWindow();
                return 0;
            }
            break;

        case PIECE_MOVING:
            stepTimer += GetFrameTime();
            if (stepTimer >= STEP_DELAY) {
                stepTimer = 0.f;
                players[currentPlayer].position += stepDir;
                stepsRemaining--;

                if (!bouncing && stepDir == 1 && players[currentPlayer].position > 100)
                {
                    int overflow = players[currentPlayer].position - 100;  
                    players[currentPlayer].position = 100;                 
                    stepsRemaining += overflow;      
                    stepDir  = -1;          
                    bouncing = true;
                    continue;      
                }

                if (stepsRemaining == 0) {
             
                    bouncing = false; stepDir = 1;
                    players[currentPlayer].position = Slide(players[currentPlayer].position);

                    personalTurn[currentPlayer]++; globalTurn++;

                    if (gMode == MODE_CHAOS && personalTurn[currentPlayer] == 2) {
                        personalTurn[currentPlayer] = 0;
                        if (snakeCount < MAX_SNAKES) canPlace[currentPlayer] = true;
                    }

                    if (players[currentPlayer].position == 100) {
                        winnerIdx = currentPlayer;
                        state = GAME_OVER;
                    }
                    else {
                        currentPlayer = (currentPlayer + 1) % playerCount;
                        state = GAME_ACTIVE;
                    }
                }
            }
            if (hit(saveB)) { saveLen = 0; saveFile[0] = '\0';
            returnState = PIECE_MOVING;
            state = NAME_INPUT_SAVE; }
            if (hit(leaveB)) { 
                ResetGame();
                state = TITLE_SCREEN;
            }
            break;

        case PLACING_SNAKE:
        {
            int ch = GetCharPressed();
            while (ch > 0) {
                if (ch >= '0' && ch <= '9' && tileLen < 3) { 
                    tileBuf[tileLen++] = (char)ch;
                    tileBuf[tileLen] = '\0';
                }
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && tileLen > 0) tileBuf[--tileLen] = '\0';
            if ((IsKeyPressed(KEY_ENTER) || hit(placeSnakeB)) && tileLen > 0) {
                int head = atoi(tileBuf);
                if (head > 1 && head < 100 && snakeCount < MAX_SNAKES && !TileOccupied(head)) 
                {
                    int tail;
                    do {
                        tail = head - GetRandomValue(5, 20);
                        if (tail < 1) tail = 1;
                    } while (TileOccupied(tail));

                    PushLink(&snakes, head, tail);
                    snakeCount++;
                    canPlace[currentPlayer] = false;
                    state = GAME_ACTIVE;
                }
                else if (TileOccupied(head)) {
                    DrawText("That tile is already occupied", 40, 80, 24, RED);
                    EndDrawing();
                    WaitTime(0.6f);
                    BeginDrawing();
                }
                else if (head > 1 && head <100 && snakeCount>MAX_SNAKES) {
                    canPlace[currentPlayer] = false;
                    DrawText("YOU HAVE REACHED THE MAX SNAKESSSSS!", 40, 80, 24, RED);
                    EndDrawing();
                    WaitTime(0.6f);
                    BeginDrawing();
                    state = GAME_ACTIVE;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                canPlace[currentPlayer] = false;
                state = GAME_ACTIVE;
            }
        } break;

        case NAME_INPUT_SAVE:
        {
            int ch = GetCharPressed();
            while (ch > 0) {
                if (ch >= 32 && ch <= 126 && saveLen < 60) { saveFile[saveLen++] = (char)ch; saveFile[saveLen] = '\0'; }
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && saveLen > 0) saveFile[--saveLen] = '\0';
            if (IsKeyPressed(KEY_ENTER) && saveLen > 0) { 
                strcat(saveFile, ".sav");
                SaveBinary(saveFile);
                state = returnState;
            }
            if (IsKeyPressed(KEY_ESCAPE)) state = returnState;
        } break;

        case GAME_OVER:
            if (IsKeyPressed(KEY_ENTER)) { ResetGame(); state = TITLE_SCREEN; }
            break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (state)
        {
        case TITLE_SCREEN:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            DrawTexture(titleB.texture, titleB.bounds.x, titleB.bounds.y, WHITE);
            DrawTexture(startB.texture, startB.bounds.x, startB.bounds.y, WHITE);
            DrawTexture(loadB.texture, loadB.bounds.x, loadB.bounds.y, WHITE);
            DrawTexture(exitB.texture, exitB.bounds.x, exitB.bounds.y, WHITE);
            break;

        case SELECT_PLAYERS:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            DrawText("Setup Game", SCREEN_WIDTH / 2 - 160, 240, 60, BLACK);

            DrawText("Players", SCREEN_WIDTH / 2 - 100, 370, 38, DARKBLUE);
            DrawText("Dice", SCREEN_WIDTH / 2 - 58, 490, 38, DARKBLUE);
            DrawText("Mode", SCREEN_WIDTH / 2 - 62, 610, 38, DARKBLUE);

            //players
            DrawTexture(twoP.texture, twoP.bounds.x, twoP.bounds.y, WHITE);
            DrawTexture(threeP.texture, threeP.bounds.x, threeP.bounds.y, WHITE);
            DrawTexture(fourP.texture, fourP.bounds.x, fourP.bounds.y, WHITE);

            //dice
            DrawTexture(oneDie.texture, oneDie.bounds.x, oneDie.bounds.y, WHITE);
            DrawTexture(twoDice.texture, twoDice.bounds.x, twoDice.bounds.y, WHITE);

            //gamemode
            DrawTexture(classicBtn.texture, classicBtn.bounds.x, classicBtn.bounds.y, WHITE);
            DrawTexture(chaosBtn.texture, chaosBtn.bounds.x, chaosBtn.bounds.y, WHITE);

            //next
            DrawTexture(playB.texture, playB.bounds.x, playB.bounds.y, WHITE);

            //HIGHLIGHTING things DrawRectangleLinesEx(bounds, thickness, & color)
            DrawRectangleLinesEx(twoP.bounds, 3, (playerCount == 2) ? RED : BLACK);
            DrawRectangleLinesEx(threeP.bounds, 3, (playerCount == 3) ? RED : BLACK);
            DrawRectangleLinesEx(fourP.bounds, 3, (playerCount == 4) ? RED : BLACK);
            DrawRectangleLinesEx(oneDie.bounds, 3, (diceCount == 1) ? RED : BLACK);
            DrawRectangleLinesEx(twoDice.bounds, 3, (diceCount == 2) ? RED : BLACK);
            DrawRectangleLinesEx(classicBtn.bounds, 4, (gMode == MODE_CLASSIC) ? GREEN : BLACK);
            DrawRectangleLinesEx(chaosBtn.bounds, 4, (gMode == MODE_CHAOS) ? GREEN : BLACK);

            DrawTexture(leaveB.texture, leaveB.bounds.x, leaveB.bounds.y, WHITE);
            break;

        case ENTER_NAMES:
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            DrawText(TextFormat("Enter name for Player %d", nameIdx + 1),
                SCREEN_WIDTH / 2 - 280, 380, 48, BLACK);
            DrawRectangleLines(SCREEN_WIDTH / 2 - 300, 460, 600, 70, BLACK);
            DrawText(nameBuf, SCREEN_WIDTH / 2 - 290, 475, 40, BLACK);
            break;

        default:
            DrawBoard();
            DrawPlayers();
            break;
        }

        if (state == GAME_ACTIVE || state == DICE_ROLLING || state == PIECE_MOVING)
        {
            DrawTexture(rollB.texture, rollB.bounds.x, rollB.bounds.y, WHITE);
            if (state != GAME_ACTIVE)
                DrawTexture(throwB.texture, throwB.bounds.x, throwB.bounds.y, WHITE);

            DrawTexture(leaveB.texture, leaveB.bounds.x, leaveB.bounds.y, WHITE);
            DrawTexture(saveB.texture, saveB.bounds.x, saveB.bounds.y, WHITE);
            if (gMode == MODE_CHAOS && canPlace[currentPlayer])
                DrawTexture(placeSnakeB.texture, placeSnakeB.bounds.x, placeSnakeB.bounds.y, WHITE);

            //dice logicc
            if (diceCount == 1)
                DrawTexture(diceTex[DiceFaceA() - 1], 1653, 90, WHITE);
            else {
                DrawTexture(diceTex[DiceFaceA() - 1], 1506, 90, WHITE);
                DrawTexture(diceTex[DiceFaceB() - 1], 1653, 90, WHITE);
            }

            DrawText(TextFormat("%s's Turn", players[currentPlayer].name),  40, 40, 32, players[currentPlayer].color);
            DrawText(TextFormat("Total Turn/s: %d", globalTurn), SCREEN_WIDTH - 260, 40, 28, BLACK);
        }

        if (state == NAME_INPUT_SAVE) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));
            DrawRectangle(SCREEN_WIDTH / 2 - 320, 380, 640, 180, WHITE);
            DrawRectangleLines(SCREEN_WIDTH / 2 - 320, 380, 640, 180, BLACK);
            DrawText("Save file name:", SCREEN_WIDTH / 2 - 300, 400, 32, BLACK);
            DrawRectangleLines(SCREEN_WIDTH / 2 - 300, 450, 600, 50, BLACK);
            DrawText(saveFile, SCREEN_WIDTH / 2 - 290, 460, 30, BLACK);
        }
        else if (state == PLACING_SNAKE) {
            DrawTexture(placeSnakeB.texture, placeSnakeB.bounds.x, placeSnakeB.bounds.y, WHITE);
            DrawRectangle(SCREEN_WIDTH - 320, 120, 300, 140, WHITE);
            DrawRectangleLines(SCREEN_WIDTH - 320, 120, 300, 140, BLACK);
            DrawText("Head tile (2-99):", SCREEN_WIDTH - 300, 135, 22, BLACK);
            DrawRectangleLines(SCREEN_WIDTH - 300, 170, 260, 40, BLACK);
            DrawText(tileBuf, SCREEN_WIDTH - 290, 178, 28, BLACK);
        }
        else if (state == GAME_OVER) {
            DrawTexture(bg.texture, bg.bounds.x, bg.bounds.y, WHITE);
            DrawText(TextFormat("%s WINS!", players[winnerIdx].name),
                SCREEN_WIDTH / 2 - 240, 340, 60, players[winnerIdx].color);
            DrawText("Press ENTER to return to title",
                SCREEN_WIDTH / 2 - 310, 440, 32, BLACK);
        }

        EndDrawing();
    }

    //just some unloading functions
    for (int i = 0; i < 6; i++) {
        UnloadTexture(diceTex[i]);
    }
    for (int i = 0; i < 4; i++) {
         UnloadTexture(tokenTex[i]);
    }
    FreeSnakesAndLadders();
    CloseWindow();
    return 0;
}