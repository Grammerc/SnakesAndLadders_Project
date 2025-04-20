#include "raylib.h"
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define BOARD_SIZE 10
#define CELL_SIZE 80

typedef enum {
    TITLE_SCREEN,
    GAME_ACTIVE,
    DICE_ROLLING,
    SETTINGS
} GameState;

typedef struct {
    Texture2D texture;
    Rectangle bounds;
} Button;

typedef struct {
    int position;
    Color color;
    char name[20];
} Player;

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

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snakes & Ladders");
    ToggleFullscreen();
    SetTargetFPS(60);
    srand(time(NULL));

    GameState state = TITLE_SCREEN;
    Player players[4];
    int playerCount = 2;
    int currentPlayer = 0;
    int diceResult = 1;
    bool diceRolling = false;
    float diceTimer = 0;
    

    Button title = createButton("B:/assets/buttons/title.png", SCREEN_WIDTH / 2 - 475, 0);
    Button background = createButton("B:/assets/buttons/background.png", SCREEN_WIDTH/2 - 960, 0);
    Button startBtn = createButton("B:/assets/buttons/start_btn.png", SCREEN_WIDTH / 2 - 100, 690);
    Button loadBtn = createButton("B:/assets/buttons/load_btn.png", SCREEN_WIDTH / 2 - 100, 800);
    Button exitBtn = createButton("B:/assets/buttons/exit_btn.png", SCREEN_WIDTH / 2 - 100, 900);
    Button rollBtn = createButton("B:/assets/buttons/roll_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    Button throwBtn = createButton("B:/assets/buttons/throw_btn.png", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 150);
    Button settingsBtn = createButton("B:/assets/buttons/settings_btn.png", 50, 50);

    Texture2D dice[6];
    for (int i = 0; i < 6; i++) {
        char path[50];
        sprintf(path, "B:/assets/dice/dice%d.png", i + 1);
        dice[i] = LoadTexture(path);
    }

    for (int i = 0; i < playerCount; i++) {
        players[i] = (Player){ 1, (Color) { rand() % 255, rand() % 255, rand() % 255, 255 }, "Player" };
        snprintf(players[i].name, sizeof(players[i].name), "Player %d", i + 1);
    }

    while (!WindowShouldClose()) {
        switch (state) {
        case TITLE_SCREEN:
            if (isButtonPressed(startBtn)) state = GAME_ACTIVE;
            if (isButtonPressed(loadBtn)) { };
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
                currentPlayer = (currentPlayer + 1) % playerCount;
                state = GAME_ACTIVE;
            }
            break;

        case SETTINGS:
            if (isButtonPressed(exitBtn)) state = TITLE_SCREEN;
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
            DrawTexture(rollBtn.texture, rollBtn.bounds.x, rollBtn.bounds.y, WHITE);
            DrawTexture(settingsBtn.texture, settingsBtn.bounds.x, settingsBtn.bounds.y, WHITE);
            DrawTexture(dice[diceResult - 1], SCREEN_WIDTH - 200, 100, WHITE);
            break;

        case DICE_ROLLING:
            DrawTexture(throwBtn.texture, throwBtn.bounds.x, throwBtn.bounds.y, WHITE);
            DrawTexture(dice[diceResult - 1], SCREEN_WIDTH - 200, 100, WHITE);
            break;

        case SETTINGS:
            DrawTexture(exitBtn.texture, exitBtn.bounds.x, exitBtn.bounds.y, WHITE);
            break;
        }
        EndDrawing();
    }

    for (int i = 0; i < 6; i++) UnloadTexture(dice[i]);
    CloseWindow();
    return 0;
}

