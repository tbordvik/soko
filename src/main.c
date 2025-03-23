#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define TILE_SIZE 32
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define HEADER_HEIGHT 30 // Space at the top for score and messages
#define EDITOR_HEIGHT 30 // Space at the bottom in editing mode.
#define BUTTON_WIDTH 30
#define SCREEN_WIDTH (TILE_SIZE * GRID_WIDTH)
#define SCREEN_HEIGHT (TILE_SIZE * GRID_HEIGHT + HEADER_HEIGHT)


typedef struct {
    unsigned char content;
} Tile;

typedef struct {
    int x;
    int y;
} Player;

// Bit masks for square content
#define WALL        0b10000
#define BOX         0b01000
#define TARGET      0b00100
#define START       0b00010
#define BOMB        0b00001

Tile current_level[GRID_WIDTH][GRID_HEIGHT] = {0};

// Game
Player player;
bool gameWon = false;
unsigned int level_num = 0;
int moves = 0;

// Editor
bool editor_mode = false;
int active_tile_selector = 0;
int content_length = 5;
bool spinner_edit_mode = false;
int spinner_val = 0; // level number


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
    // Not sure if this will work on windows
    sprintf(filename, "resources/levels/level%d", level_num); 
    if(!FileExists(filename)) {
        for(int x=0; x<GRID_WIDTH; x++) {
            for(int y=0; y<GRID_HEIGHT; y++) {
                current_level[x][y].content = 0;
            }
        }
        return;
    }
    FILE *fp = fopen(filename, "r");
    unsigned char level_bytes[GRID_WIDTH][GRID_HEIGHT];
    fread(level_bytes, sizeof(level_bytes), 1, fp);
    fclose(fp);

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
    sprintf(filename, "resources/levels/level%d", level_num);
    FILE *fp = fopen(filename, "w");
    fwrite(bytes_to_save, sizeof(bytes_to_save), 1, fp);
    fclose(fp);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sokoban Game");
    SetTargetFPS(60);

    Texture2D wall = LoadTexture("resources/wall32.png");
    Texture2D target = LoadTexture("resources/target32.png");
    Texture2D box = LoadTexture("resources/box32.png");
    Texture2D player_texture = LoadTexture("resources/player32.png");
    Rectangle tile_src = {0, 0, TILE_SIZE, TILE_SIZE};

    loadLevel();

    char debug_string[128];
    /*for(int x=0; x<GRID_WIDTH; x++) {*/
    /*    for(int y=0; y<GRID_HEIGHT; y++) {*/
    /*        sprintf(debug_string, "[%d][%d]: %d\n", x, y, (int) current_level[x][y].content); */
    /*        TraceLog(LOG_WARNING, debug_string);*/
    /*    }*/
    /*}*/

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
            int new_x = player.x + dx;
            int new_y = player.y + dy;
            if(dx != 0 || dy != 0) {

                if (new_x >= 0 && new_y < GRID_WIDTH && new_y >= 0 && new_y < GRID_HEIGHT) {
                    unsigned char pushed_content = current_level[new_x][new_y].content;
                    // Check walls blocking movement
                    if (!(pushed_content & WALL)) {

                        // The pushed item's new pos (maybe)
                        int item_new_x = new_x + dx;
                        int item_new_y = new_y + dy;
                        unsigned char pushed_to_content = current_level[item_new_x][item_new_y].content;

                        if (item_new_x >= 0 && item_new_x < GRID_WIDTH && item_new_y >= 0 && item_new_y < GRID_HEIGHT) {
                            // Box pushing
                            if (pushed_content & BOX) {
                                bool can_push_box = !(pushed_to_content & WALL || pushed_to_content & BOX || pushed_to_content & BOMB); 
                                if (can_push_box) {
                                    current_level[item_new_x][item_new_y].content |= BOX;
                                    current_level[new_x][new_y].content &= ~BOX; 
                                    player.x = new_x;
                                    player.y = new_y;
                                    moves = moves + 1;

                                    // Check win condition after pushing a box
                                    if (isGameWon()) {
                                        gameWon = true;
                                    }
                                }
                            }
                            // Bomb pushing
                            else if (pushed_content & BOMB) {
                                if (!(pushed_to_content & BOX || pushed_to_content & BOMB)) {
                                    if(pushed_to_content & WALL) {
                                        // KABOOM!
                                        current_level[item_new_x][item_new_y].content = 0;
                                    }
                                    else {
                                        current_level[item_new_x][item_new_y].content |= BOMB;
                                    }
                                    current_level[new_x][new_y].content &= ~BOMB;
                                    player.x = new_x;
                                    player.y = new_y;
                                    moves = moves + 1;
                                }
                            }

                            else {
                                player.x = new_x;
                                player.y = new_y;
                                moves = moves + 1;
                            }

                        }
                    }
                }
            }
        }
        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw header stuff
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
 
                if(current_level[x][y].content & BOMB) {
                    /*DrawTextureRec(wall, tile_src, (Vector2){tileX, tileY}, WHITE);*/
                    DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, RED);
                }
                if (!(current_level[x][y].content & (BOX | TARGET | WALL | BOMB ))) {
                    // Draw grid lines
                    /*DrawRectangleLines(tileX, tileY, TILE_SIZE, TILE_SIZE, DARKGRAY);*/

                }
            }
        }
        // Draw player
        DrawTextureRec(player_texture, tile_src, (Vector2){player.x * TILE_SIZE, player.y * TILE_SIZE + HEADER_HEIGHT}, PINK);

        unsigned char selected_tile;
        if(editor_mode) {
            Rectangle tile_select = {0, SCREEN_HEIGHT - EDITOR_HEIGHT, BUTTON_WIDTH, EDITOR_HEIGHT};

            // Can we use emojis here?
            int wtf = GuiToggleGroup(tile_select, "EMPTY;BOMB;START;TARGET;BOX;WALL", &active_tile_selector);

            // active_tile_selector is 0, 1, ..., N. But we want the bit mask representation.
            if(active_tile_selector == 0) {
                selected_tile = 0;
            }
            else {
                selected_tile = (unsigned char) (1 << (active_tile_selector - 1));
            }

            Rectangle save_button_rect = {SCREEN_WIDTH - BUTTON_WIDTH, SCREEN_HEIGHT - EDITOR_HEIGHT, BUTTON_WIDTH, EDITOR_HEIGHT};
            GuiButton(save_button_rect, "Save");

            // Level selector / loader
            if(GuiSpinner((Rectangle){(SCREEN_WIDTH - 100) / 2.0f, SCREEN_HEIGHT - EDITOR_HEIGHT, 100, EDITOR_HEIGHT}, "Level", &spinner_val, 0, 10, spinner_edit_mode)) spinner_edit_mode = !spinner_edit_mode;
            GuiButton((Rectangle) { SCREEN_WIDTH / 2.0f + 50.0f, SCREEN_HEIGHT - EDITOR_HEIGHT, BUTTON_WIDTH, EDITOR_HEIGHT}, "Load");

            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 cursor = GetMousePosition();
                bool save_button_is_clicked = cursor.x > SCREEN_WIDTH - BUTTON_WIDTH && cursor.y > SCREEN_HEIGHT - EDITOR_HEIGHT;
                bool load_button_is_clicked = (cursor.x > SCREEN_WIDTH / 2.0f + 50.0f) && (cursor.x < SCREEN_WIDTH / 2.0f + 50.0f + (float) BUTTON_WIDTH) && cursor.y > SCREEN_HEIGHT - EDITOR_HEIGHT;
                if(save_button_is_clicked) {
                    saveLevel();
                }
                if(load_button_is_clicked) {
                    level_num = spinner_val;
                    loadLevel();
                }
            }

            // Draw new tile
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 cursor = GetMousePosition();
                bool toggle_group_is_clicked = (cursor.x < BUTTON_WIDTH*content_length + content_length * 10.0f || cursor.x > SCREEN_WIDTH - BUTTON_WIDTH) && (cursor.y > (SCREEN_HEIGHT - EDITOR_HEIGHT)); 
                bool save_button_is_clicked = cursor.x > SCREEN_WIDTH - BUTTON_WIDTH && cursor.y > SCREEN_HEIGHT - EDITOR_HEIGHT;
                if(!toggle_group_is_clicked && !save_button_is_clicked) {
                    int gridX = cursor.x / TILE_SIZE;
                    int gridY = (cursor.y - HEADER_HEIGHT) / TILE_SIZE;
                    if(selected_tile & START) {
                        // Remove previous starting pos so we only have one
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
            // TODO: this drawing is not good.
            DrawText("You Win! Press Space for next level!", SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2, 30, YELLOW);
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
            TraceLog(LOG_DEBUG, debug_string);
        }
    }
    UnloadTexture(box);
    UnloadTexture(player_texture);
    UnloadTexture(target);
    UnloadTexture(wall);
    CloseWindow();
    return 0;
}
