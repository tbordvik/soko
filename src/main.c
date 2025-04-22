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
// TODO: Find a better way to set the screen size.
#define SCREEN_WIDTH (TILE_SIZE * GRID_WIDTH)
#define SCREEN_HEIGHT (TILE_SIZE * GRID_HEIGHT + HEADER_HEIGHT)

// Bit masks for square content
#define FLAMES      0b0100000
#define WALL        0b0010000
#define BOX         0b0001000
#define TARGET      0b0000100
#define START       0b0000010
#define BOMB        0b0000001

#define MAX_SNAPSHOTS   1024
Tile level[GRID_WIDTH][GRID_HEIGHT] = {0};

// Game
Player player;
bool game_won = false;
int level_num = 0;
int moves = 0;
int buffered_key;

// Editor
bool editor_mode = false;
int active_tile_selector = 0;
int content_length = 5;
bool spinner_edit_mode = false;
int spinner_val = 0; // level number
bool is_restartable_and_loadable = true;

Tile snapshots[MAX_SNAPSHOTS][GRID_WIDTH][GRID_HEIGHT];
int num_snapshots = 0;

bool is_game_won() {
    int targets = 0;
    int boxes_on_target = 0;
    for (int x = 0; x < GRID_HEIGHT; x++) {
        for (int y = 0; y < GRID_WIDTH; y++) {
            if (level[x][y].content & TARGET) {
                targets++;
                if (level[x][y].content & BOX) {
                    boxes_on_target++;
                }
            }
        }
    }
    return targets > 0 && targets == boxes_on_target;
}

Player start_pos() {
    for(int x=0; x<GRID_WIDTH; x++) {
        for(int y=0; y<GRID_HEIGHT; y++) {
            if(level[x][y].content & START) {
                Player p = { x, y, x, y, false};
                return p;
            }
        }
    }
    Player p = { 0, 0, 0, 0, false };
    return p;
    /*assert(false && "Must have a starting position in the level");*/
}

void reset_snapshots() {
    num_snapshots = 1;
    memcpy(&snapshots[0], &level, sizeof(level));
}

void add_snapshot() {
    memcpy(&snapshots[num_snapshots++], &level, sizeof(level));
}

void pop_snapshot() {
    memcpy(&level, &snapshots[--num_snapshots], sizeof(level)); 
}

void load_level() {
    char filename[32];
    // Not sure if this will work on windows
    sprintf(filename, "resources/levels/level%d", level_num); 
    if(!FileExists(filename)) {
        for(int x=0; x<GRID_WIDTH; x++) {
            for(int y=0; y<GRID_HEIGHT; y++) {
                level[x][y] = (Tile) { 0, x, y, x, y, false, 0 };
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
        level[x][y] = (Tile) { lvl_bytes[i], x, y, x, y, false, 0 };
    }
    player = start_pos();
    moves = 0;
    reset_snapshots();
}

void save_level() {
    unsigned char bytes_to_save[GRID_WIDTH][GRID_HEIGHT];
    for(int x=0; x<GRID_WIDTH; x++) {
        for(int y=0; y<GRID_HEIGHT; y++) {
            bytes_to_save[x][y] = level[x][y].content;
        }
    }
    char filename[32];
    sprintf(filename, "resources/levels/level%d", level_num);
    SaveFileData(filename, bytes_to_save, sizeof(bytes_to_save));
}

Vector2 title_pos = { -400, 250 };
Vector2 subtitle_pos = { 180, SCREEN_HEIGHT + 100 };

void show_intro_screen() {
    float delta = GetFrameTime();
    float animation_time = 1.5f;
    char* title = "Sokoban Clone";
    char* subtitle = "Press Space to start!";
    bool show_subtitle = false;

    title_pos.x = title_pos.x + delta * 500 / animation_time; 
    if(title_pos.x > 200) {
        title_pos.x = 200;
        subtitle_pos.y = subtitle_pos.y - delta * 280 / animation_time;
        show_subtitle = true;
        if(subtitle_pos.y < 400) {
            subtitle_pos.y = 400;
        }
    }

    DrawText(title, title_pos.x, title_pos.y, 32, ORANGE);
    DrawText(subtitle, subtitle_pos.x, subtitle_pos.y, 24, ORANGE);
}

void move_player(int new_x, int new_y) {
        level[player.x][player.y].content &= ~START;
        level[new_x][new_y].content |= START;
        player.x = new_x;
        player.y = new_y;
        player.is_animating = true;
        moves = moves + 1;
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sokoban Game");
    SetTargetFPS(60);

    InitAudioDevice();
    Music melody = LoadMusicStream("resources/fetish_all_over.mp3");
    PlayMusicStream(melody);

    Texture2D wall = LoadTexture("resources/wall32.png");
    Texture2D target = LoadTexture("resources/target32.png");
    Texture2D box = LoadTexture("resources/box32.png");
    Texture2D player_texture = LoadTexture("resources/player32.png");
    Texture2D bomb = LoadTexture("resources/tnt32.png");
    Rectangle tile_src = {0, 0, TILE_SIZE, TILE_SIZE};

    Texture2D flames = LoadTexture("resources/firesheet_32.png");
    Rectangle flames_rect[5*5];
    for(int y=0; y<5; y++) {
        for(int x=0; x<5; x++) {
            flames_rect[y * 5 + x] = (Rectangle) { x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE }; 
        }
    }

    load_level();

    char debug_string[128] = "";
    /*for(int x=0; x<GRID_WIDTH; x++) {*/
    /*    for(int y=0; y<GRID_HEIGHT; y++) {*/
    /*        sprintf(debug_string, "[%d][%d]: %d\n", x, y, (int) level[x][y].content); */
    /*        TraceLog(LOG_WARNING, debug_string);*/
    /*    }*/

    /*}*/
    bool show_intro = true;
    while (!WindowShouldClose()) {
        float delta = GetFrameTime();
        UpdateMusicStream(melody);
        // Game Logic
        if (!show_intro && !game_won && !editor_mode) {
            // Input handling
            int dx = 0, dy = 0;
            if(player.is_animating) {
                // TODO: Maybe reverse animation if the direction pressed is opposite of where we are going?
                // Nah.. think that will be weird when pushing things.
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) buffered_key = KEY_RIGHT;
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) buffered_key = KEY_LEFT;
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) buffered_key = KEY_UP;
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) buffered_key = KEY_DOWN;
            }
            else {
                if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)|| buffered_key == KEY_RIGHT) dx = 1;
                else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || buffered_key == KEY_LEFT) dx = -1;
                else if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) || buffered_key == KEY_UP) dy = -1;
                else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S) || buffered_key == KEY_DOWN) dy = 1;
                if(IsKeyPressed(KEY_Z)) {
                    if(num_snapshots > 0) {
                        pop_snapshot();
                        player = start_pos();
                    }
                }
                buffered_key = 0;
            }
            // Move player
            int new_x = player.x + dx;
            int new_y = player.y + dy;
            if(dx != 0 || dy != 0) {

                if (new_x >= 0 && new_y < GRID_WIDTH && new_y >= 0 && new_y < GRID_HEIGHT) {
                    unsigned char pushed_content = level[new_x][new_y].content;
                    // Check walls blocking movement
                    if (!(pushed_content & WALL)) {

                        // The pushed item's new pos (maybe)
                        int item_new_x = new_x + dx;
                        int item_new_y = new_y + dy;
                        unsigned char pushed_to_content = level[item_new_x][item_new_y].content;

                        if (item_new_x >= 0 && item_new_x < GRID_WIDTH && item_new_y >= 0 && item_new_y < GRID_HEIGHT) {
                            // Box pushing
                            if (pushed_content & BOX) {
                                bool can_push_box = !(pushed_to_content & WALL || pushed_to_content & BOX || pushed_to_content & BOMB); 
                                if (can_push_box) {
                                    add_snapshot();
                                    Tile *from = &level[new_x][new_y];
                                    Tile *to = &level[item_new_x][item_new_y];
                                    from->content &= ~BOX; 
                                    to->content |= BOX;
                                    to->display_x = new_x;
                                    to->display_y = new_y;
                                    to->is_animating = true;
                                    move_player(new_x, new_y);
                                    game_won = is_game_won();
                                }
                            }
                            // Bomb pushing
                            else if (pushed_content & BOMB) {
                                if (!(pushed_to_content & BOX)) {
                                    add_snapshot();
                                    Tile *from = &level[new_x][new_y];
                                    Tile *to = &level[item_new_x][item_new_y];
                                    if(pushed_to_content & WALL) {
                                        // KABOOM!
                                        to->content = 0;
                                        to->content |= FLAMES;
                                    }
                                    else if(pushed_to_content & BOMB) {
                                        // SuperKABOOM!
                                        if(item_new_x < GRID_WIDTH) {
                                            level[item_new_x + 1][item_new_y].content &= ~WALL;
                                            level[item_new_x + 1][item_new_y].content |= FLAMES;
                                        }
                                        if(item_new_x > 0) {
                                            level[item_new_x - 1][item_new_y].content &= ~WALL;
                                            level[item_new_x - 1][item_new_y].content |= FLAMES;
                                        }
                                        if(item_new_y > 0) {
                                            level[item_new_x][item_new_y - 1].content &= ~WALL;
                                            level[item_new_x][item_new_y - 1].content |= FLAMES;
                                        }
                                        if(item_new_y < GRID_HEIGHT) {
                                            level[item_new_x][item_new_y + 1].content &= ~WALL;
                                            level[item_new_x][item_new_y + 1].content |= FLAMES;
                                        }
                                        to->content &= ~BOMB;
                                    }
                                    else {
                                        to->content |= BOMB;
                                    }
                                    from->content &= ~BOMB;
                                    to->display_x = new_x;
                                    to->display_y = new_y;
                                    to->is_animating = true;
                                    move_player(new_x, new_y);
                                }
                            }

                            else {
                                add_snapshot();
                                move_player(new_x, new_y);
                            }
                        }
                    }
                }
            }
        }
        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);

        if(show_intro) {
            show_intro_screen();
            if(IsKeyPressed(KEY_SPACE)) {
                show_intro = false;
            }
        }
        else {
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

            // Grid
            for (int y = 0; y < GRID_HEIGHT; y++) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    Tile *tile = &level[x][y];
                    animate_tile(tile);
                    int tile_x = tile->display_x * TILE_SIZE;
                    int tile_y = tile->display_y * TILE_SIZE + HEADER_HEIGHT;
                    Vector2 pos = { tile_x, tile_y };
                    Vector2 immovable_pos = { tile->grid_x * TILE_SIZE, tile->grid_y * TILE_SIZE + HEADER_HEIGHT};

                    // Draw tile contents
                    if (tile->content & TARGET) {
                        DrawTextureRec(target, tile_src, immovable_pos, WHITE);
                    }
                    if(tile->content & WALL) {
                        DrawTextureRec(wall, tile_src, immovable_pos, WHITE);
                    }
                    if (tile->content & BOX) {
                        DrawTextureRec(box, tile_src, pos, WHITE);
                    }
                    if(tile->content & BOMB) {
                        DrawTextureRec(bomb, tile_src, pos, WHITE);
                    }
                    if(tile->content & FLAMES) {
                        if(tile->animation_frame < sizeof(flames_rect) / sizeof(flames_rect[0]) ) {
                            DrawTextureRec(flames, flames_rect[tile->animation_frame++], immovable_pos, WHITE);
                        }
                        else {
                            tile->content &= ~FLAMES;
                        }
                    }
                    if (!(tile->content & (BOX | TARGET | WALL | BOMB ))) {
                        // Draw grid lines
                        /*DrawRectangleLines(tileX, tileY, TILE_SIZE, TILE_SIZE, DARKGRAY);*/

                    }
                }
            }
            animate_player(&player);
            DrawTextureRec(player_texture, tile_src, (Vector2){player.display_x * TILE_SIZE, player.display_y * TILE_SIZE + HEADER_HEIGHT}, PINK);

            unsigned char selected_tile;
            if(editor_mode) {
                Rectangle tile_select = {0, 0, BUTTON_WIDTH, HEADER_HEIGHT};

                // Can we use emojis here?
                int wtf = GuiToggleGroup(tile_select, "NADA;BOMB;ME;END;BOX;WALL", &active_tile_selector);
                if(IsKeyPressed(KEY_ONE)) {
                    active_tile_selector = 0;
                }
                if(IsKeyPressed(KEY_TWO)) {
                    active_tile_selector = 1;
                }
                if(IsKeyPressed(KEY_THREE)) {
                    active_tile_selector = 2;
                }
                if(IsKeyPressed(KEY_FOUR)) {
                    active_tile_selector = 3;
                }
                if(IsKeyPressed(KEY_FIVE)) {
                    active_tile_selector = 4;
                }
                if(IsKeyPressed(KEY_SIX)) {
                    active_tile_selector = 5;
                }
                // active_tile_selector is 0, 1, ..., N. But we want the bit mask representation.
                if(active_tile_selector == 0) {
                    selected_tile = 0;
                }
                else {
                    selected_tile = (unsigned char) (1 << (active_tile_selector - 1));
                }

                Rectangle save_button_rect = {SCREEN_WIDTH - BUTTON_WIDTH, 0, BUTTON_WIDTH, HEADER_HEIGHT};
                GuiButton(save_button_rect, "Save");
                Rectangle checkbox_rect = { (float) SCREEN_WIDTH * 2 / 3, 0, BUTTON_WIDTH, HEADER_HEIGHT};
                GuiCheckBox(checkbox_rect, "Enable RS/Load", &is_restartable_and_loadable);

                // Level selector / loader
                if(GuiSpinner((Rectangle){(SCREEN_WIDTH - 100) / 2.0f, 0, 100, HEADER_HEIGHT}, "Level", &spinner_val, 0, 10, spinner_edit_mode)) spinner_edit_mode = !spinner_edit_mode;
                GuiButton((Rectangle) { SCREEN_WIDTH / 2.0f + 50.0f, 0, BUTTON_WIDTH, HEADER_HEIGHT}, "Load");

                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 cursor = GetMousePosition();
                    bool header_is_clicked = cursor.y < HEADER_HEIGHT;
                    bool save_button_is_clicked = cursor.x > SCREEN_WIDTH - BUTTON_WIDTH && header_is_clicked;
                    bool load_button_is_clicked = (cursor.x > SCREEN_WIDTH / 2.0f + 50.0f) && (cursor.x < SCREEN_WIDTH / 2.0f + 50.0f + (float) BUTTON_WIDTH) && header_is_clicked;
                    if(save_button_is_clicked) {
                        save_level();
                    }
                    if(load_button_is_clicked && is_restartable_and_loadable) {
                        level_num = spinner_val;
                        load_level();
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
                                    level[x][y].content &= ~START;
                                }
                            }
                            player.x = player.display_x = gridX;
                            player.y = player.display_y = gridY;
                        }
                        level[gridX][gridY].content = selected_tile;
                    }
               }
            }
            else {
                if (game_won) {
                    if(level_num == 6) {
                        DrawText("You beat da game! GG!", 150, SCREEN_HEIGHT / 2 - 50, 32, YELLOW);
                    }
                    else {
                        DrawText("Press Space for next level!", 150, SCREEN_HEIGHT / 2 - 50, 24, YELLOW);
                        if(IsKeyPressed(KEY_SPACE)) {
                            level_num++;
                            game_won = false;
                            load_level();
                        }
                    }
                }
            }
            if(IsKeyPressed(KEY_R) && is_restartable_and_loadable) {
                load_level();
                game_won = false;
            }
            if(IsKeyPressed(KEY_E)) {
                editor_mode = !editor_mode;
            }
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
    UnloadMusicStream(melody);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
