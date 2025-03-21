#include "raylib.h"
#include <string.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define TILE_SIZE 64
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define HEADER_HEIGHT 30 // Space at the top for score and messages
#define SCREEN_WIDTH (TILE_SIZE * GRID_WIDTH)
#define SCREEN_HEIGHT (TILE_SIZE * GRID_HEIGHT + HEADER_HEIGHT)

// Tile structure with walls and content
typedef struct {
    unsigned char content;
} Tile;

// Player position
typedef struct {
    int x;
    int y;
} Player;

int moves = 0;

// Bit masks for square content
#define WALL        0b1000
#define BOX         0b0100
#define TARGET      0b0010
/*#define WALL  0b0100*/
/*#define WALL 0b0010*/
/*#define WALL   0b0001// Simple level (0 = empty, 1 = wall, 2 = player start, 3 = box, 4 = target)*/

Tile level[GRID_HEIGHT][GRID_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, BOX, 0, 0, 0},
    {0, 0, WALL, WALL, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, TARGET, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

Player player = {2, 1};
bool gameWon = false;

bool isGameWon() {
    int targets = 0;
    int boxesOnTargets = 0;
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (level[y][x].content & TARGET) {
                targets++;
                if (level[y][x].content & BOX) {
                    boxesOnTargets++;
                }
            }
        }
    }
    return targets > 0 && targets == boxesOnTargets;
}

void restartLevel(void* currentLevel) {
    memcpy(level, currentLevel, sizeof(level));
    player.x = 2;
    player.y = 1;
    moves = 0;
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sokoban Game");
    SetTargetFPS(60);

    Tile currentLevel[GRID_HEIGHT][GRID_WIDTH];
    restartLevel(&currentLevel);
    while (!WindowShouldClose()) {
        if (!gameWon) {
            // Input handling
            int dx = 0, dy = 0;
            if (IsKeyPressed(KEY_RIGHT)) dx = 1;
            if (IsKeyPressed(KEY_LEFT)) dx = -1;
            if (IsKeyPressed(KEY_UP)) dy = -1;
            if (IsKeyPressed(KEY_DOWN)) dy = 1;

            // Move player
            int newX = player.x + dx;
            int newY = player.y + dy;
            if(dx != 0 || dy != 0) {

                if (newX >= 0 && newX < GRID_WIDTH && newY >= 0 && newY < GRID_HEIGHT) {
                    // Check walls blocking movement
                    bool canMove = true;
                    if (dx == 1 && level[newY][newX].content & WALL) canMove = false;
                    if (dx == -1 && level[newY][newX].content & WALL) canMove = false;
                    if (dy == 1 && level[newY][newX].content & WALL) canMove = false;
                    if (dy == -1 && level[newY][newX].content & WALL) canMove = false;

                    if (canMove) {
                        // Check for box pushing
                        if (level[newY][newX].content & BOX) {
                            int boxNewX = newX + dx;
                            int boxNewY = newY + dy;
                            if (boxNewX >= 0 && boxNewX < GRID_WIDTH && boxNewY >= 0 && boxNewY < GRID_HEIGHT) {
                                bool canPush = true;
                                if (dx == 1 && level[boxNewY][boxNewX].content & WALL) canPush = false;
                                if (dx == -1 && level[boxNewY][boxNewX].content & WALL) canPush = false;
                                if (dy == 1 && level[boxNewY][boxNewX].content & WALL) canPush = false;
                                if (dy == -1 && level[boxNewY][boxNewX].content & WALL) canPush = false;

                                // Allow pushing onto EMPTY or TARGET, but not another BOX
                                if (canPush && !(level[boxNewY][boxNewX].content & BOX)) {
                                    level[boxNewY][boxNewX].content |= BOX; // Add BOX to new position
                                    level[newY][newX].content &= ~BOX;      // Remove BOX from old position
                                    player.x = newX;
                                    player.y = newY;
                                    moves = moves + 1;

                                    // Check win condition after pushing a box
                                    if (isGameWon()) {
                                        gameWon = true;
                                    }
                                }
                            }
                        } else if (!(level[newY][newX].content & BOX)) { // Player can move to EMPTY or TARGET
                            player.x = newX;
                            player.y = newY;
                            moves = moves + 1;
                        }
                    }
                }
            }
        }
        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw header area (strictly within HEADER_HEIGHT)
        DrawRectangle(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, DARKGRAY);
        char scoreText[32];
        sprintf(scoreText, "Moves: %d", moves);
        DrawText(scoreText, 10, 10, 20, WHITE); // Smaller font size to fit
        char restartText[32] = "Press R to restart";
        DrawText(restartText, 290, 10, 17, WHITE);

        // Draw game grid (shifted down by HEADER_HEIGHT)
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int tileX = x * TILE_SIZE;
                int tileY = y * TILE_SIZE + HEADER_HEIGHT;

                // Draw tile contents
                if (level[y][x].content & TARGET) {
                    DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, GREEN);
                }
                if (level[y][x].content & BOX) {
                    DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, BROWN);
                }
                if(level[y][x].content & WALL) {
                    DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, RED);
                }
                if (!(level[y][x].content & (BOX | TARGET | WALL))) {
                    DrawRectangleLines(tileX, tileY, TILE_SIZE, TILE_SIZE, DARKGRAY);
                }
            }
        }
        // Draw player with proper offset
        DrawRectangle(player.x * TILE_SIZE, player.y * TILE_SIZE + HEADER_HEIGHT, TILE_SIZE, TILE_SIZE, BLUE);

        if (gameWon) {
            DrawText("You Win!", SCREEN_WIDTH / 2 - MeasureText("You Win!", 30) / 2, 40, 30, YELLOW);
        }

        if(IsKeyPressed(KEY_R)) {
            restartLevel(&currentLevel);
            gameWon = false;
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
