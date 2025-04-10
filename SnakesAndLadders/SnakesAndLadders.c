#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

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

void getTimeBasedFilename(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, size, "snakes_ladders_save_%Y%m%d.txt", t);
}

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
        printf("\033[2J\033[H");

        printf("\033[1;32m");
        printf("   _____             _             \n");
        printf("  / ____|           | |            \n");
        printf(" | (___  _ __   __ _| | _____  ___ \n");
        printf("  \\___ \\| '_ \\ / _` | |/ / _ \\/ __|\n");
        printf("  ____) | | | | (_| |   <  __/\\__ \\\n");
        printf(" |_____/|_| |_|\\__,_|_|\\_\\___||___/\n");

        printf("\033[1;33m");
        printf("                  _               \n");
        printf("                 | |              \n");
        printf("   __ _ _ __   __| |              \n");
        printf("  / _` | '_ \\ / _` |              \n");
        printf(" | (_| | | | | (_| |              \n");
        printf("  \\__,_|_| |_|\\__,_|              \n");
        printf("\033[1;36m");
        printf("  _               _     _               \n");
        printf(" | |             | |   | |              \n");
        printf(" | |     __ _  __| | __| | ___ _ __ ___ \n");
        printf(" | |    / _` |/ _` |/ _` |/ _ \\ '__/ __|\n");
        printf(" | |___| (_| | (_| | (_| |  __/ |  \\__ \\\n");
        printf(" |______\\__,_|\\__,_|\\__,_|\\___|_|  |___/\n\n");

        printf("\033[0m");
        printf("\t1. Start New Game\n");
        printf("\t2. Load Game\n");
        printf("\t3. Exit\n\n");
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
            exit(0);
            break;
        default:
            printf("Invalid choice. Please try again.\n");
            break;
        }
    }
}

void startNewGame() {
    printf("\nHow many players?: ");
    scanf("%d", &totalPlayers);
    clearBuffer();
    if (totalPlayers < 1) {
        printf("Invalid number of players!\n");
        return;
    }
    players = (Player*)malloc(sizeof(Player) * totalPlayers);
    if (!players) {
        printf("Memory allocation failed!\n");
        exit(0);
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
    char fileName[100];
    char fullPath[300];

    printf("\nAvailable save files:\n");
    system("dir /b \"C:\\Users\\Raphael\\source\\repos\\SnakesAndLadders_Project\\SnakesAndLadders\\Saved Files\\*.txt\"");

    printf("\nEnter the filename to load:");
    scanf("%s", fileName);
    clearBuffer();

    snprintf(fullPath, sizeof(fullPath),
        "C:\\Users\\Raphael\\source\\repos\\SnakesAndLadders_Project\\SnakesAndLadders\\Saved Files\\%s", fileName);

    FILE* fp = fopen(fullPath, "r");
    if (!fp) {
        printf("\nNo saved game found.\n");
        return;
    }

    if (fscanf(fp, "%d", &totalPlayers) != 1 || totalPlayers < 1) {
        fclose(fp);
        printf("Invalid data in save file.\n");
        return;
    }

    players = (Player*)malloc(sizeof(Player) * totalPlayers);
    if (!players) {
        printf("Memory allocation failed!\n");
        fclose(fp);
        exit(0);
    }

    for (int i = 0; i < totalPlayers; i++) {
        if (fscanf(fp, "%s %d %d", players[i].name, &players[i].position, &players[i].hasWon) != 3) {
            fclose(fp);
            printf("Invalid player data in save file.\n");
            free(players);
            players = NULL;
            return;
        }
    }

    if (fscanf(fp, "%d", &gameBoard.boardSize) != 1) {
        fclose(fp);
        printf("Invalid board data in save file.\n");
        free(players);
        players = NULL;
        return;
    }

    if (fscanf(fp, "%d", &gameBoard.snakeCount) != 1) {
        fclose(fp);
        printf("Invalid snake count.\n");
        free(players);
        players = NULL;
        return;
    }

    for (int i = 0; i < gameBoard.snakeCount; i++) {
        if (fscanf(fp, "%d %d", &gameBoard.snakeHead[i], &gameBoard.snakeTail[i]) != 2) {
            fclose(fp);
            printf("Invalid snake data.\n");
            free(players);
            players = NULL;
            return;
        }
    }

    if (fscanf(fp, "%d", &gameBoard.ladderCount) != 1) {
        fclose(fp);
        printf("Invalid ladder count.\n");
        free(players);
        players = NULL;
        return;
    }

    for (int i = 0; i < gameBoard.ladderCount; i++) {
        if (fscanf(fp, "%d %d", &gameBoard.ladderStart[i], &gameBoard.ladderEnd[i]) != 2) {
            fclose(fp);
            printf("Invalid ladder data.\n");
            free(players);
            players = NULL;
            return;
        }
    }

    fclose(fp);
    printf("\nGame loaded successfully!\n\n");
    playGame();
}

void saveGame() {
    char fileName[200];
    char userInput[100];

    printf("Enter Save File Name: ");
    if (fgets(userInput, sizeof(userInput), stdin)) {
        size_t len = strlen(userInput);
        if (len > 0 && userInput[len - 1] == '\n') {
            userInput[len - 1] = '\0';
        }
    }

    snprintf(fileName, sizeof(fileName), "Saved Files\\%s.txt", userInput);

    FILE* fp = fopen(fileName, "w");
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
    printf("\nGame saved to '%s' successfully!\n", fileName);
}

void playGame() {
    int currentPlayerIndex = 0;
    int gameFinished = 0;
    while (!gameFinished) {
        printBoard();
        Player* p = &players[currentPlayerIndex];
        if (!p->hasWon) {
            printf("\n[%s]'s turn (at tile %d). Press ENTER to roll dice, or type 's' to save, or 'q' to quit: ", p->name, p->position);
            char userInput[10];
            if (fgets(userInput, sizeof(userInput), stdin)) {
                size_t len = strlen(userInput);
                if (len > 0 && userInput[len - 1] == '\n') {
                    userInput[len - 1] = '\0';
                }
            }
            if (strcmp(userInput, "s") == 0 || strcmp(userInput, "S") == 0) {
                saveGame();
            }
            else if (strcmp(userInput, "q") == 0 || strcmp(userInput, "Q") == 0) {
                printf("\nExiting the game...\n");
                return;
            }
            else {
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
    for (int row = 10; row >= 1; row--) {
        for (int col = 1; col <= 10; col++) {
            int tileNumber = (row - 1) * 10 + col;
            int playerIndexOnTile = -1;
            int foundSnakeHeadIndex = -1;
            int foundSnakeTailIndex = -1;
            int foundLadderStartIndex = -1;
            int foundLadderEndIndex = -1;
            for (int p = 0; p < totalPlayers; p++) {
                if (players[p].position == tileNumber) {
                    playerIndexOnTile = p;
                    break;
                }
            }
            for (int i = 0; i < gameBoard.snakeCount; i++) {
                if (gameBoard.snakeHead[i] == tileNumber) {
                    foundSnakeHeadIndex = i;
                    break;
                }
                if (gameBoard.snakeTail[i] == tileNumber) {
                    foundSnakeTailIndex = i;
                    break;
                }
            }
            for (int i = 0; i < gameBoard.ladderCount; i++) {
                if (gameBoard.ladderStart[i] == tileNumber) {
                    foundLadderStartIndex = i;
                    break;
                }
                if (gameBoard.ladderEnd[i] == tileNumber) {
                    foundLadderEndIndex = i;
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
            else if (foundSnakeHeadIndex != -1) {
                printf("\033[41m[SH]\033[0m");
            }
            else if (foundSnakeTailIndex != -1) {
                printf("\033[41m[ST]\033[0m");
            }
            else if (foundLadderStartIndex != -1) {
                printf("\033[42m[LS]\033[0m");
            }
            else if (foundLadderEndIndex != -1) {
                printf("\033[42m[LE]\033[0m");
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
    int nextPos = p->position + dice;
    if (nextPos > 100) {
        nextPos = 100;
    }
    p->position = nextPos;
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