#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define TILE_SIZE 64
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define HEADER_HEIGHT 30 // Space at the top for score and messages
#define EDITOR_HEIGHT 30 // Space at the bottom in editing mode.
#define BUTTON_WIDTH 60
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
// Bit masks for square content
#define WALL        0b10000
#define BOX         0b01000
#define TARGET      0b00100
#define START       0b00010
Tile level[GRID_WIDTH][GRID_HEIGHT] = {0};
Tile current_level[GRID_WIDTH][GRID_HEIGHT] = {0};

Player player;
bool gameWon = false;
int level_num = 0;
int moves = 0;
bool editor_mode = false;
int active_tile_selector = 0;


bool isGameWon() {
    int targets = 0;
    int boxesOnTargets = 0;
    for (int x = 0; x < GRID_HEIGHT; x++) {
        for (int y = 0; y < GRID_WIDTH; y++) {
            if (current_level[x][y].content & TARGET) {
                targets++;
                if (current_level[x][y].content & BOX) {
                    boxesOnTargets++;
                }
            }
        }
    }
    return targets > 0 && targets == boxesOnTargets;
}

Player startPos() {
    for(int x=0; x<GRID_WIDTH; x++) {
        for(int y=0; y<GRID_HEIGHT; y++) {
            if(current_level[x][y].content & START) {
                Player p = { x, y };
                return p;
            }
        }
    }
    Player p = { 0, 0 };
    return p;
    /*assert(false && "Must have a starting position in the level");*/
}

void loadLevel() {
    char filename[32];
    sprintf(filename, "level%d", level_num); 
    if(!FileExists(filename)) {
        for(int x=0; x<GRID_WIDTH; x++) {
            for(int y=0; y<GRID_HEIGHT; y++) {
                current_level[x][y].content = 0;
            }
        }
        return;
    }
    FILE *fp = fopen(filename, "r");
    /*assert(fp != 0);*/
    unsigned char level_bytes[GRID_WIDTH][GRID_HEIGHT];
    fread(level_bytes, sizeof(level_bytes), 1, fp);
    fclose(fp);

    printf("Size: %zu", sizeof(level_bytes));
    char debug_str[128];
    for(int x = 0; x<GRID_WIDTH; x++) {
        for(int y = 0; y<GRID_HEIGHT; y++) {

            /*sprintf(debug_str, "[%d][%d] - %d", x, y, level_bytes[x][y]);*/
            /*TraceLog(LOG_WARNING, debug_str);*/
            current_level[x][y].content = level_bytes[x][y];
        }
    }
    player = startPos();
    moves = 0;
}

void saveLevel() {
    unsigned char bytes_to_save[GRID_WIDTH][GRID_HEIGHT];
    for(int x=0; x<GRID_WIDTH; x++) {
        for(int y=0; y<GRID_HEIGHT; y++) {
            bytes_to_save[x][y] = current_level[x][y].content;
        }
    }
    char filename[32];
    sprintf(filename, "level%d", level_num);
    FILE *fp = fopen(filename, "w");
    fwrite(bytes_to_save, sizeof(bytes_to_save), 1, fp);
    fclose(fp);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sokoban Game");
    SetTargetFPS(60);
    loadLevel();

    Texture2D wall = LoadTexture("resources/wall.png");
    Texture2D target = LoadTexture("resources/target.png");
    Texture2D box = LoadTexture("resources/box.png");
    Texture2D player_texture = LoadTexture("resources/player.png");
    Rectangle tile_src = {0, 0, TILE_SIZE, TILE_SIZE};
    TraceLog(LOG_WARNING, "asdf");
    char debug_string[128];
    for(int x=0; x<GRID_WIDTH; x++) {
        for(int y=0; y<GRID_HEIGHT; y++) {
            sprintf(debug_string, "[%d][%d]: %d\n", x, y, (int) current_level[x][y].content); 
            /*TraceLog(LOG_WARNING, debug_string);*/
        }
    }

    // restartLevel();
    // restartLevel(&current_level);
    while (!WindowShouldClose()) {
        // Game Logic
        if (!gameWon && !editor_mode) {
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
                    if (dx == 1 && current_level[newX][newY].content & WALL) canMove = false;
                    if (dx == -1 && current_level[newX][newY].content & WALL) canMove = false;
                    if (dy == 1 && current_level[newX][newY].content & WALL) canMove = false;
                    if (dy == -1 && current_level[newX][newY].content & WALL) canMove = false;

                    if (canMove) {
                        // Check for box pushing
                        if (current_level[newX][newY].content & BOX) {
                            int boxNewX = newX + dx;
                            int boxNewY = newY + dy;
                            if (boxNewX >= 0 && boxNewX < GRID_WIDTH && boxNewY >= 0 && boxNewY < GRID_HEIGHT) {
                                bool canPush = true;
                                if (dx == 1 && current_level[boxNewX][boxNewY].content & WALL) canPush = false;
                                if (dx == -1 && current_level[boxNewX][boxNewY].content & WALL) canPush = false;
                                if (dy == 1 && current_level[boxNewX][boxNewY].content & WALL) canPush = false;
                                if (dy == -1 && current_level[boxNewX][boxNewY].content & WALL) canPush = false;

                                // Allow pushing onto EMPTY or TARGET, but not another BOX
                                if (canPush && !(current_level[boxNewX][boxNewY].content & BOX)) {
                                    current_level[boxNewX][boxNewY].content |= BOX; // Add BOX to new position
                                    current_level[newX][newY].content &= ~BOX;      // Remove BOX from old position
                                    player.x = newX;
                                    player.y = newY;
                                    moves = moves + 1;

                                    // Check win condition after pushing a box
                                    if (isGameWon()) {
                                        gameWon = true;
                                    }
                                }
                            }
                        } else if (!(current_level[newX][newY].content & BOX)) { // Player can move to EMPTY or TARGET
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

        char levelText[32];
        sprintf(levelText, "Level %d", level_num + 1);
        DrawText(levelText, (SCREEN_WIDTH / 2) - 20, 10, 20, WHITE);

        char restartText[32] = "R to restart";
        DrawText(restartText, SCREEN_WIDTH - 150, 10, 17, BROWN);
        // Draw game grid (shifted down by HEADER_HEIGHT)
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int tileX = x * TILE_SIZE;
                int tileY = y * TILE_SIZE + HEADER_HEIGHT;

                // Draw tile contents
                if (current_level[x][y].content & TARGET) {
                    DrawTextureRec(target, tile_src, (Vector2){tileX, tileY}, WHITE);
                    /*DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, GREEN);*/
                }
                if (current_level[x][y].content & BOX) {
                    DrawTextureRec(box, tile_src, (Vector2){tileX, tileY}, WHITE);
                    /*DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, BROWN);*/
                }
                if(current_level[x][y].content & WALL) {
                    DrawTextureRec(wall, tile_src, (Vector2){tileX, tileY}, WHITE);
                    /*DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, RED);*/
                }
                if (!(current_level[x][y].content & (BOX | TARGET | WALL))) {
                    /*DrawRectangleLines(tileX, tileY, TILE_SIZE, TILE_SIZE, DARKGRAY);*/

                }
            }
        }
        // Draw player with proper offset
        DrawTextureRec(player_texture, tile_src, (Vector2){player.x * TILE_SIZE, player.y * TILE_SIZE + HEADER_HEIGHT}, PINK);

        unsigned char selected_tile;
        if(editor_mode) {
            Rectangle tile_select = {0, SCREEN_HEIGHT - EDITOR_HEIGHT, BUTTON_WIDTH, EDITOR_HEIGHT};
            int wtf = GuiToggleGroup(tile_select, "EMPTY;START;TARGET;BOX;WALL", &active_tile_selector);
            selected_tile = (char) (1 << active_tile_selector);
            Rectangle save_button_rect = {SCREEN_WIDTH - BUTTON_WIDTH, SCREEN_HEIGHT - EDITOR_HEIGHT, BUTTON_WIDTH, EDITOR_HEIGHT};
            GuiButton(save_button_rect, "Save");
            GuiButton( (Rectangle){SCREEN_WIDTH - BUTTON_WIDTH*2, SCREEN_HEIGHT - EDITOR_HEIGHT, BUTTON_WIDTH, EDITOR_HEIGHT}, "New");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 cursor = GetMousePosition();
                bool save_button_is_clicked = cursor.x > SCREEN_WIDTH - BUTTON_WIDTH && cursor.y > SCREEN_HEIGHT - EDITOR_HEIGHT;
                if(save_button_is_clicked) {
                    saveLevel();
                }
            }
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 cursor = GetMousePosition();
                bool toggle_group_is_clicked = (cursor.x < 310 || cursor.x > SCREEN_WIDTH - BUTTON_WIDTH) && (cursor.y > (SCREEN_HEIGHT - EDITOR_HEIGHT)); 
                bool save_button_is_clicked = cursor.x > SCREEN_WIDTH - BUTTON_WIDTH && cursor.y > SCREEN_HEIGHT - EDITOR_HEIGHT;
                if(!toggle_group_is_clicked && !save_button_is_clicked) {
                    int gridX = cursor.x / TILE_SIZE;
                    int gridY = (cursor.y - HEADER_HEIGHT) / TILE_SIZE;
                    // Remove previous starting pos so we only have one
                    if(selected_tile & START) {
                        for(int x=0; x<GRID_WIDTH; x++) {
                            for(int y=0; y<GRID_HEIGHT; y++) {
                                current_level[x][y].content &= ~START;
                            }
                        }
                        player.x = gridX;
                        player.y = gridY;
                    }
                    current_level[gridX][gridY].content = selected_tile;
                }
           }
        }
        if (gameWon) {
            DrawText("You Win! Press Space for next level!", SCREEN_WIDTH / 2 - MeasureText("You Win!", 30) / 2, 40, 30, YELLOW);
            if(IsKeyPressed(KEY_SPACE)) {
                level_num++;
                gameWon = false;
                loadLevel();
            }
        }

        if(IsKeyPressed(KEY_R)) {
            loadLevel();
            gameWon = false;
        }
        if(IsKeyPressed(KEY_E)) {
            editor_mode = !editor_mode;
        }
        EndDrawing();
        if(strlen(debug_string) > 0) {
            /*puts(debug_string);*/
        }
    }
    CloseWindow();
    return 0;
}
