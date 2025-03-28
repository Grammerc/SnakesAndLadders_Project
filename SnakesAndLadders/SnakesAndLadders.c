#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

typedef struct {
    char name[50];
    int position;
    int hasWon;
} Player;

typedef struct {
    int boardSize;
    int snakeCount;
    int ladderCount;
    int snakeHead[10];
    int snakeTail[10];
    int ladderStart[10];
    int ladderEnd[10];
} Board;

Board gameBoard;
Player* players = NULL;
int totalPlayers = 0;
const char* SAVE_FILE = "snakes_ladders_save.txt";

void mainMenu();
void startNewGame();
void loadGame();
void saveGame();
void playGame();
void initializeBoard();
void printBoard();
int rollDice();
void movePlayer(Player* p, int dice);
void clearBuffer();

int main(void) {
    srand((unsigned)time(NULL));
    mainMenu();
    if (players != NULL) {
        free(players);
        players = NULL;
    }
    return 0;
}

void mainMenu() {
    int choice = 0;
    while (choice != 3) {
        printf("\n========================================\n");
        printf(" S N A K E S   A N D   L A D D E R S\n");
        printf("========================================\n");
        printf("1. Start New Game\n");
        printf("2. Load Game\n");
        printf("3. Exit\n");
        printf("========================================\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        clearBuffer();
        switch (choice) {
        case 1:
            startNewGame();
            break;
        case 2:
            loadGame();
            break;
        case 3:
            printf("\nExiting the game...\n");
            break;
        default:
            printf("Invalid choice. Please try again.\n");
            break;
        }
    }
}

void startNewGame() {
    printf("\nHow many players? ");
    scanf("%d", &totalPlayers);
    clearBuffer();
    if (totalPlayers < 1) {
        printf("Invalid number of players!\n");
        return;
    }
    players = (Player*)malloc(sizeof(Player) * totalPlayers);
    if (!players) {
        printf("Memory allocation failed!\n");
        exit(1);
    }
    for (int i = 0; i < totalPlayers; i++) {
        printf("Enter name for Player %d: ", i + 1);
        fgets(players[i].name, 50, stdin);
        {
            size_t len = strlen(players[i].name);
            if (len > 0 && players[i].name[len - 1] == '\n') {
                players[i].name[len - 1] = '\0';
            }
        }
        players[i].position = 1;
        players[i].hasWon = 0;
    }
    initializeBoard();
    playGame();
}

void loadGame() {
    FILE* fp = fopen(SAVE_FILE, "r");
    if (!fp) {
        printf("\nNo saved game found or error opening file.\n");
        return;
    }
    fscanf(fp, "%d", &totalPlayers);
    if (totalPlayers < 1) {
        fclose(fp);
        printf("Invalid data in save file.\n");
        return;
    }
    players = (Player*)malloc(sizeof(Player) * totalPlayers);
    if (!players) {
        printf("Memory allocation failed!\n");
        fclose(fp);
        exit(1);
    }
    for (int i = 0; i < totalPlayers; i++) {
        fscanf(fp, "%s %d %d", players[i].name, &players[i].position, &players[i].hasWon);
    }
    fscanf(fp, "%d", &gameBoard.boardSize);
    fscanf(fp, "%d", &gameBoard.snakeCount);
    for (int i = 0; i < gameBoard.snakeCount; i++) {
        fscanf(fp, "%d %d", &gameBoard.snakeHead[i], &gameBoard.snakeTail[i]);
    }
    fscanf(fp, "%d", &gameBoard.ladderCount);
    for (int i = 0; i < gameBoard.ladderCount; i++) {
        fscanf(fp, "%d %d", &gameBoard.ladderStart[i], &gameBoard.ladderEnd[i]);
    }
    fclose(fp);
    printf("\nGame loaded successfully!\n\n");
    playGame();
}

void saveGame() {
    FILE* fp = fopen(SAVE_FILE, "w");
    if (!fp) {
        printf("Error opening file for saving.\n");
        return;
    }
    fprintf(fp, "%d\n", totalPlayers);
    for (int i = 0; i < totalPlayers; i++) {
        fprintf(fp, "%s %d %d\n", players[i].name, players[i].position, players[i].hasWon);
    }
    fprintf(fp, "%d\n", gameBoard.boardSize);
    fprintf(fp, "%d\n", gameBoard.snakeCount);
    for (int i = 0; i < gameBoard.snakeCount; i++) {
        fprintf(fp, "%d %d\n", gameBoard.snakeHead[i], gameBoard.snakeTail[i]);
    }
    fprintf(fp, "%d\n", gameBoard.ladderCount);
    for (int i = 0; i < gameBoard.ladderCount; i++) {
        fprintf(fp, "%d %d\n", gameBoard.ladderStart[i], gameBoard.ladderEnd[i]);
    }
    fclose(fp);
    printf("\nGame saved successfully!\n");
}

void playGame() {
    int currentPlayerIndex = 0;
    int gameFinished = 0;
    while (!gameFinished) {
        printBoard();
        Player* p = &players[currentPlayerIndex];
        if (!p->hasWon) {
            printf("\n[%s]'s turn (currently at tile %d). Press ENTER to roll dice...", p->name, p->position);
            getchar();
            {
                int dice = rollDice();
                printf("%s rolled a %d!\n", p->name, dice);
                movePlayer(p, dice);
                if (p->position >= 100) {
                    p->position = 100;
                    p->hasWon = 1;
                    printf("\nCONGRATULATIONS! %s has reached tile 100!\n", p->name);
                    gameFinished = 1;
                }
            }
        }
        currentPlayerIndex = (currentPlayerIndex + 1) % totalPlayers;
    }
    printf("\nFinal positions:\n");
    for (int i = 0; i < totalPlayers; i++) {
        printf("%s - Tile %d\n", players[i].name, players[i].position);
    }
}

void initializeBoard() {
    gameBoard.boardSize = 10;
    gameBoard.snakeCount = 2;
    gameBoard.snakeHead[0] = 14;
    gameBoard.snakeTail[0] = 7;
    gameBoard.snakeHead[1] = 99;
    gameBoard.snakeTail[1] = 78;
    gameBoard.ladderCount = 2;
    gameBoard.ladderStart[0] = 4;
    gameBoard.ladderEnd[0] = 25;
    gameBoard.ladderStart[1] = 40;
    gameBoard.ladderEnd[1] = 59;
}

void printBoard() {
    int row, col;
    for (row = 10; row >= 1; row--) {
        for (col = 1; col <= 10; col++) {
            int tileNumber = (row - 1) * 10 + col;
            int playerIndexOnTile = -1;
            int foundSnakeIndex = -1;
            int foundLadderIndex = -1;
            for (int p = 0; p < totalPlayers; p++) {
                if (players[p].position == tileNumber) {
                    playerIndexOnTile = p;
                    break;
                }
            }
            for (int i = 0; i < gameBoard.snakeCount; i++) {
                if (gameBoard.snakeHead[i] == tileNumber) {
                    foundSnakeIndex = i;
                    break;
                }
            }
            for (int i = 0; i < gameBoard.ladderCount; i++) {
                if (gameBoard.ladderStart[i] == tileNumber) {
                    foundLadderIndex = i;
                    break;
                }
            }

            printf("\t");
            if (playerIndexOnTile != -1) {
                const char* colors[] = {
                    "\033[31m",
                    "\033[32m",
                    "\033[33m",
                    "\033[34m",
                    "\033[35m",
                    "\033[36m"
                };
                const char* reset = "\033[0m";
                const char* color = colors[playerIndexOnTile % 6];
                printf("%s[P%d]%s", color, playerIndexOnTile + 1, reset);
            }
            else if (foundSnakeIndex != -1) {
                printf("\033[41m[S ]\033[0m");
            }
            else if (foundLadderIndex != -1) {
                printf("\033[42m[L ]\033[0m");
            }
            else {
                printf("[%02d]", tileNumber);
            }
        }
        printf("\n\n");
    }
}

int rollDice() {
    for (int i = 0; i < 8; i++) {
        int temp = (rand() % 6) + 1;
        printf("Rolling... %d\r", temp);
        fflush(stdout);
        Sleep(300);
    }
    int finalRoll = (rand() % 6) + 1;
    printf("Final roll: %d\n", finalRoll);
    return finalRoll;
}

void movePlayer(Player* p, int dice) {
    p->position += dice;
    if (p->position > 100) {
        p->position = 100;
    }
    for (int i = 0; i < gameBoard.snakeCount; i++) {
        if (p->position == gameBoard.snakeHead[i]) {
            printf("Oh no! %s hit a snake at %d. Sliding down to %d.\n", p->name, gameBoard.snakeHead[i], gameBoard.snakeTail[i]);
            p->position = gameBoard.snakeTail[i];
        }
    }
    for (int i = 0; i < gameBoard.ladderCount; i++) {
        if (p->position == gameBoard.ladderStart[i]) {
            printf("Great! %s hit a ladder at %d. Climbing up to %d.\n", p->name, gameBoard.ladderStart[i], gameBoard.ladderEnd[i]);
            p->position = gameBoard.ladderEnd[i];
        }
    }
}

void clearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}