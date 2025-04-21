#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define SCREEN_WIDTH 1920  
#define SCREEN_HEIGHT 1080
#define BOARD_SIZE 10
#define CELL_SIZE 80

#define BOARD_OFFSET_X (SCREEN_WIDTH - BOARD_SIZE * CELL_SIZE) / 2 
#define BOARD_OFFSET_Y 150

#define MAX_SNAKES 5
#define MAX_LADDERS 5

typedef enum {
    TITLE_SCREEN,
    GAME_ACTIVE,
    DICE_ROLLING,
    SETTINGS,
    GAME_OVER
} GameState;

typedef struct {
    Texture2D texture;
    Rectangle bounds;
} Button;

typedef struct {
    int position;
    Color color;
    char name[20];
    int playerNumber;
} Player;

typedef struct {
    int start;
    int end;
} SnakeOrLadder;

int boardNumbers[BOARD_SIZE][BOARD_SIZE];
SnakeOrLadder snakes[MAX_SNAKES];
SnakeOrLadder ladders[MAX_LADDERS];

Button createButton(const char* path, int x, int y) {
    Button btn;
    btn.texture = LoadTexture(path);
    btn.bounds = (Rectangle){ x, y, btn.texture.width, btn.texture.height };
    return btn;
}

bool isButtonPressed(Button btn) {
    return CheckCollisionPointRec(GetMousePosition(), btn.bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

int rollDice() {
    return (GetRandomValue(1, 6));
}

void InitBoardNumbers() {
    int counter = 1;
    bool goingRight = true;

    for (int row = BOARD_SIZE - 1; row >= 0; row--) {
        if (goingRight) {
            for (int col = 0; col < BOARD_SIZE; col++) {
                boardNumbers[row][col] = counter++;
            }
        }
        else {
            for (int col = BOARD_SIZE - 1; col >= 0; col--) {
                boardNumbers[row][col] = counter++;
            }
        }
        goingRight = !goingRight;
    }
}

void InitSnakesAndLadders() {
    snakes[0] = (SnakeOrLadder){ 16, 6 };
    snakes[1] = (SnakeOrLadder){ 37, 21 };
    snakes[2] = (SnakeOrLadder){ 49, 27 };
    snakes[3] = (SnakeOrLadder){ 82, 55 };
    snakes[4] = (SnakeOrLadder){ 97, 78 };

    ladders[0] = (SnakeOrLadder){ 4, 14 };
    ladders[1] = (SnakeOrLadder){ 8, 30 };
    ladders[2] = (SnakeOrLadder){ 28, 76 };
    ladders[3] = (SnakeOrLadder){ 71, 92 };
    ladders[4] = (SnakeOrLadder){ 80, 99 };
}

Vector2 GetCellPosition(int cellNumber) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (boardNumbers[row][col] == cellNumber) {
                return (Vector2) {
                    BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2,
                        BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2
                };
            }
        }
    }
    return (Vector2) { 0, 0 };
}

int CheckSnakeOrLadder(int position) {
    for (int i = 0; i < MAX_SNAKES; i++) {
        if (snakes[i].start == position) {
            return snakes[i].end;
        }
    }

    for (int i = 0; i < MAX_LADDERS; i++) {
        if (ladders[i].start == position) {
            return ladders[i].end;
        }
    }

    return position;
}

void DrawPlayers(Player players[], int playerCount) {
    for (int i = 0; i < playerCount; i++) {
        if (players[i].position < 1 || players[i].position > BOARD_SIZE * BOARD_SIZE) continue;

        Vector2 pos = GetCellPosition(players[i].position);
        float radius = CELL_SIZE / 4;
        float angle = (2 * PI * i) / playerCount;
        pos.x += cos(angle) * radius * 0.6;
        pos.y += sin(angle) * radius * 0.6;

        DrawCircleV(pos, radius, players[i].color);

        char playerNum[3];
        sprintf(playerNum, "%d", players[i].playerNumber);
        DrawText(playerNum, pos.x - 5, pos.y - 10, 20, BLACK);
    }
}

void DrawSnakesAndLadders() {
    for (int i = 0; i < MAX_SNAKES; i++) {
        Vector2 start = GetCellPosition(snakes[i].start);
        Vector2 end = GetCellPosition(snakes[i].end);
        DrawLineEx(start, end, 5, RED);
    }

    for (int i = 0; i < MAX_LADDERS; i++) {
        Vector2 start = GetCellPosition(ladders[i].start);
        Vector2 end = GetCellPosition(ladders[i].end);
        DrawLineEx(start, end, 5, GREEN);
    }
}

void DrawBoard() {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Color cellColor = ((row + col) % 2 == 0) ? LIGHTGRAY : WHITE;
            DrawRectangle(
                BOARD_OFFSET_X + col * CELL_SIZE,
                BOARD_OFFSET_Y + row * CELL_SIZE,
                CELL_SIZE,
                CELL_SIZE,
                cellColor
            );

            char numText[5];
            sprintf(numText, "%d", boardNumbers[row][col]);
            DrawText(
                numText,
                BOARD_OFFSET_X + col * CELL_SIZE + 10,
                BOARD_OFFSET_Y + row * CELL_SIZE + 10,
                20,
                DARKGRAY
            );

            DrawRectangleLines(
                BOARD_OFFSET_X + col * CELL_SIZE,
                BOARD_OFFSET_Y + row * CELL_SIZE,
                CELL_SIZE,
                CELL_SIZE,
                BLACK
            );
        }
    }
}

int main() {
    SetTraceLogLevel(LOG_ALL);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snakes & Ladders");
    srand(time(NULL));
    InitBoardNumbers();
    InitSnakesAndLadders();

    GameState state = TITLE_SCREEN;
    Player players[4];
    int playerCount = 2;
    int currentPlayer = 0;
    int diceResult = 1;
    bool diceRolling = false;
    float diceTimer = 0;


    Button title = createButton("C:/assets/buttons/title.png", SCREEN_WIDTH / 2 - 400, 150);
    Button background = createButton("C:/assets/buttons/background.png", SCREEN_WIDTH / 2 - 960, 0);
    Button startBtn = createButton("C:/assets/buttons/start_btn.png", SCREEN_WIDTH / 2 - 100, 690);
    Button loadBtn = createButton("C:/assets/buttons/load_btn.png", SCREEN_WIDTH / 2 - 100, 800);
    Button exitBtn = createButton("C:/assets/buttons/exit_btn.png", SCREEN_WIDTH / 2 - 100, 900);
    Button rollBtn = createButton("C:/assets/buttons/roll_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    Button throwBtn = createButton("C:/assets/buttons/throw_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    Button settingsBtn = createButton("C:/assets/buttons/settings_btn.png", 50, 400);

    Texture2D dice[6];
    for (int i = 0; i < 6; i++) {
        char path[50];
        sprintf(path, "C:/assets/dice/dice%d.png", i + 1);
        dice[i] = LoadTexture(path);
    }

    for (int i = 0; i < playerCount; i++) {
        players[i] = (Player){ 1, (Color) { rand() % 255, rand() % 255, rand() % 255, 255 }, "Player", i + 1 };
        snprintf(players[i].name, sizeof(players[i].name), "Player %d", i + 1);
    }

    while (!WindowShouldClose()) {
        switch (state) {
        case TITLE_SCREEN:
            if (isButtonPressed(startBtn)) state = GAME_ACTIVE;
            if (isButtonPressed(loadBtn)) {};
            if (isButtonPressed(exitBtn)) CloseWindow();
            if (isButtonPressed(settingsBtn)) state = SETTINGS;
            break;

        case GAME_ACTIVE:
            if (isButtonPressed(rollBtn)) {
                diceRolling = true;
                diceTimer = 0;
                state = DICE_ROLLING;
            }
            if (isButtonPressed(settingsBtn)) state = SETTINGS;
            break;

        case DICE_ROLLING:
            diceTimer += GetFrameTime();
            if (diceTimer >= 0.1f) {
                diceResult = rollDice();
                diceTimer = 0;
            }
            if (isButtonPressed(throwBtn)) {
                diceRolling = false;
                players[currentPlayer].position += diceResult;

                if (players[currentPlayer].position > BOARD_SIZE * BOARD_SIZE) {
                    players[currentPlayer].position = BOARD_SIZE * BOARD_SIZE - (players[currentPlayer].position - BOARD_SIZE * BOARD_SIZE);
                }

                int newPos = CheckSnakeOrLadder(players[currentPlayer].position);
                if (newPos != players[currentPlayer].position) {
                    players[currentPlayer].position = newPos;
                }

                if (players[currentPlayer].position == BOARD_SIZE * BOARD_SIZE) {
                    state = GAME_OVER;
                }
                else {
                    currentPlayer = (currentPlayer + 1) % playerCount;
                    state = GAME_ACTIVE;
                }
            }
            break;

        case SETTINGS:
            if (isButtonPressed(exitBtn)) state = TITLE_SCREEN;
            break;

        case GAME_OVER:
            if (IsKeyPressed(KEY_ENTER)) {
                for (int i = 0; i < playerCount; i++) {
                    players[i].position = 1;
                }
                currentPlayer = 0;
                state = TITLE_SCREEN;
            }
            break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (state) {
        case TITLE_SCREEN:
            DrawTexture(background.texture, background.bounds.x, background.bounds.y, WHITE);
            DrawTexture(title.texture, title.bounds.x, title.bounds.y, WHITE);
            DrawTexture(startBtn.texture, startBtn.bounds.x, startBtn.bounds.y, WHITE);
            DrawTexture(loadBtn.texture, loadBtn.bounds.x, loadBtn.bounds.y, WHITE);
            DrawTexture(exitBtn.texture, exitBtn.bounds.x, exitBtn.bounds.y, WHITE);
            DrawTexture(settingsBtn.texture, settingsBtn.bounds.x, settingsBtn.bounds.y, WHITE);
            break;

        case GAME_ACTIVE:
            DrawBoard();
            DrawSnakesAndLadders();
            DrawPlayers(players, playerCount);
            DrawTexture(rollBtn.texture, rollBtn.bounds.x, rollBtn.bounds.y, WHITE);
            DrawTexture(settingsBtn.texture, settingsBtn.bounds.x, settingsBtn.bounds.y, WHITE);
            DrawTexture(dice[diceResult - 1], SCREEN_WIDTH - 200, 100, WHITE);

            char currentPlayerText[20];
            sprintf(currentPlayerText, "Current: %s", players[currentPlayer].name);
            DrawText(currentPlayerText, 50, 100, 30, players[currentPlayer].color);
            break;

        case DICE_ROLLING:
            DrawBoard();
            DrawSnakesAndLadders();
            DrawPlayers(players, playerCount);
            DrawTexture(throwBtn.texture, throwBtn.bounds.x, throwBtn.bounds.y, WHITE);
            DrawTexture(dice[diceResult - 1], SCREEN_WIDTH - 200, 100, WHITE);

            sprintf(currentPlayerText, "Current: %s", players[currentPlayer].name);
            DrawText(currentPlayerText, 50, 100, 30, players[currentPlayer].color);
            break;

        case SETTINGS:
            DrawTexture(exitBtn.texture, exitBtn.bounds.x, exitBtn.bounds.y, WHITE);
            break;

        case GAME_OVER:
            DrawText("GAME OVER", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 50, 50, RED);
            for (int i = 0; i < playerCount; i++) {
                if (players[i].position == BOARD_SIZE * BOARD_SIZE) {
                    char winnerText[50];
                    sprintf(winnerText, "Winner: %s", players[i].name);
                    DrawText(winnerText, SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 50, 40, players[i].color);
                    break;
                }
            }
            DrawText("Press ENTER to return to title", SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 + 150, 30, BLACK);
            break;
        }
        EndDrawing();
    }

    for (int i = 0; i < 6; i++) UnloadTexture(dice[i]);
    CloseWindow();
    return 0;
}