#include "animation.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"


#define TILE_SIZE 32
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define HEADER_HEIGHT 30 // Space at the top for game info / editor
#define BUTTON_WIDTH 35
#define SCREEN_WIDTH (TILE_SIZE * GRID_WIDTH)
#define SCREEN_HEIGHT (TILE_SIZE * GRID_HEIGHT + HEADER_HEIGHT)

typedef struct {
    // Maybe 2-3 stored keystrokes is sufficient to store.. and if the user 
    // inputs more we overwrite the last one? So
    //
    // Hmm, try just storing one first and see how that goes? don't need a stack
    // for that tho..
    char key[32]; 
    int top;
} Stack;

bool is_empty(Stack *stack) {
    return stack->top == -1;
}

char pop(Stack *stack) {
    if(is_empty(stack)) {
        return NULL;
    }
    char popped = stack->key[stack->top];
    stack->top--;
    return popped;
}

void push(Stack *stack, char to_push) {
    if(stack->top == 31) {
        return;
    }
    stack->key[++stack->top] = to_push;
}

typedef struct {
    unsigned char content;
} Tile;
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
int level_num = 0;
int moves = 0;
int buffered_key;

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
                Player p = { x, y, x, y, false};
                return p;
            }
        }
    }
    Player p = { 0, 0, 0, 0, false };
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
    unsigned char *lvl_bytes;
    int size = 0;
    lvl_bytes = LoadFileData(filename, &size);

    // File data is stored in order (0, 0), (0, 1), ..., (1, 0), (1, 1) ...
    for(int i=0; i<size; i++) {
        int x = i / GRID_WIDTH;
        int y = i % GRID_HEIGHT;
        current_level[x][y].content = lvl_bytes[i];
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
    SaveFileData(filename, bytes_to_save, sizeof(bytes_to_save));
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sokoban Game");
    SetTargetFPS(60);

    Texture2D wall = LoadTexture("resources/wall32.png");
    Texture2D target = LoadTexture("resources/target32.png");
    Texture2D box = LoadTexture("resources/box32.png");
    Texture2D player_texture = LoadTexture("resources/player32.png");
    Texture2D bomb = LoadTexture("resources/tnt32.png");
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

        float delta = GetFrameTime();
        // Game Logic
        if (!gameWon && !editor_mode) {
            // Input handling
            int dx = 0, dy = 0;
            if(player.is_animating) {
                if (IsKeyPressed(KEY_RIGHT)) buffered_key = KEY_RIGHT;
                if (IsKeyPressed(KEY_LEFT)) buffered_key = KEY_LEFT;
                if (IsKeyPressed(KEY_UP)) buffered_key = KEY_UP;
                if (IsKeyPressed(KEY_DOWN)) buffered_key = KEY_DOWN;
            }
            else {
                if (IsKeyPressed(KEY_RIGHT)) dx = 1;
                else if (IsKeyPressed(KEY_LEFT)) dx = -1;
                else if (IsKeyPressed(KEY_UP)) dy = -1;
                else if (IsKeyPressed(KEY_DOWN)) dy = 1;
            }
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
                                    player.is_animating = true;
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
                                    player.is_animating = true;
                                    moves = moves + 1;
                                }
                            }

                            else {
                                player.x = new_x;
                                player.y = new_y;
                                player.is_animating = true;
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
        if(!editor_mode) {
            char scoreText[32];
            sprintf(scoreText, "Moves: %d", moves);
            DrawText(scoreText, 10, 8, 20, WHITE);

            char levelText[32];
            sprintf(levelText, "Level %d", level_num + 1);
            DrawText(levelText, (SCREEN_WIDTH / 2) - 20, 8, 20, WHITE);

            char restartText[32] = "R to restart";
            DrawText(restartText, SCREEN_WIDTH - 150, 8, 17, BROWN);
        }
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
                    DrawTextureRec(bomb, tile_src, (Vector2){tileX, tileY}, WHITE);
                    /*DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, RED);*/
                }
                if (!(current_level[x][y].content & (BOX | TARGET | WALL | BOMB ))) {
                    // Draw grid lines
                    /*DrawRectangleLines(tileX, tileY, TILE_SIZE, TILE_SIZE, DARKGRAY);*/

                }
            }
        }
        animate_player(&player);
        DrawTextureRec(player_texture, tile_src, (Vector2){player.displayX * TILE_SIZE, player.displayY * TILE_SIZE + HEADER_HEIGHT}, PINK);

        unsigned char selected_tile;
        if(editor_mode) {
            Rectangle tile_select = {0, 0, BUTTON_WIDTH, HEADER_HEIGHT};

            // Can we use emojis here?
            int wtf = GuiToggleGroup(tile_select, "NADA;BOMB;ME;END;BOX;WALL", &active_tile_selector);

            // active_tile_selector is 0, 1, ..., N. But we want the bit mask representation.
            if(active_tile_selector == 0) {
                selected_tile = 0;
            }
            else {
                selected_tile = (unsigned char) (1 << (active_tile_selector - 1));
            }

            Rectangle save_button_rect = {SCREEN_WIDTH - BUTTON_WIDTH, 0, BUTTON_WIDTH, HEADER_HEIGHT};
            GuiButton(save_button_rect, "Save");

            // Level selector / loader
            if(GuiSpinner((Rectangle){(SCREEN_WIDTH - 100) / 2.0f, 0, 100, HEADER_HEIGHT}, "Level", &spinner_val, 0, 10, spinner_edit_mode)) spinner_edit_mode = !spinner_edit_mode;
            GuiButton((Rectangle) { SCREEN_WIDTH / 2.0f + 50.0f, 0, BUTTON_WIDTH, HEADER_HEIGHT}, "Load");

            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 cursor = GetMousePosition();
                bool header_is_clicked = cursor.y < HEADER_HEIGHT;
                bool save_button_is_clicked = cursor.x > SCREEN_WIDTH - BUTTON_WIDTH && header_is_clicked;
                bool load_button_is_clicked = (cursor.x > SCREEN_WIDTH / 2.0f + 50.0f) && (cursor.x < SCREEN_WIDTH / 2.0f + 50.0f + (float) BUTTON_WIDTH) && header_is_clicked;
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
                bool header_is_clicked = cursor.y < HEADER_HEIGHT;
                if(!header_is_clicked) {
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
        else {
            if (gameWon) {
                DrawText("Press Space for next level!", 150, SCREEN_HEIGHT / 2 - 50, 24, YELLOW);
                if(IsKeyPressed(KEY_SPACE)) {
                    level_num++;
                    gameWon = false;
                    loadLevel();
                }
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
            TraceLog(LOG_WARNING, debug_string);
        }
    }
    UnloadTexture(box);
    UnloadTexture(player_texture);
    UnloadTexture(target);
    UnloadTexture(wall);
    UnloadTexture(bomb);
    CloseWindow();
    return 0;
}
