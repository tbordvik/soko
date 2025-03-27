#include "raylib.h"
#include "animation.h"

#define ANIM_TIME 0.15f // In seconds.

// Calculate the current animation frame
void animate_player(Player *player) {
float delta = GetFrameTime();
if(player->is_animating) {
    float dist = delta / ANIM_TIME;
    // Must be a better way to do this lol..
    // but to tired right now to simplify.
    if(player->x > player->displayX) {
        player->displayX += dist;
        if(player->displayX >= player->x) {
            player->displayX = player->x;
            player->is_animating = false;
        }
    } else if (player->x < player->displayX) {
        player->displayX -= dist;
        if(player->displayX <= player->x) {
            player->displayX = player->x;
            player->is_animating = false;
        }
    }
    if(player->y > player->displayY) {
        player->displayY += dist;
        if(player->displayY >= player->y) {
            player->displayY = player->y;
            player->is_animating = false;
        }
    }
    else if (player->y < player->displayY) {
        player->displayY -= dist;
        if(player->displayY <= player->y) {
            player->displayY = player->y;
            player->is_animating = false;
        }
    }
}
}
// Calculate the current animation frame
void animate_tile(Tile *tile) {
float delta = GetFrameTime();
float animation_time = 0.2f;
if(tile->is_animating) {
    float dist = delta / ANIM_TIME;
    // Must be a better way to do this lol..
    // but to tired right now to simplify.
    if(tile->grid_x > tile->display_x) {
        tile->display_x += dist;
        if(tile->display_x >= tile->grid_x) {
            tile->display_x = tile->grid_x;
            tile->is_animating = false;
        }
    } else if (tile->grid_x < tile->display_x) {
        tile->display_x -= dist;
        if(tile->display_x <= tile->grid_x) {
            tile->display_x = tile->grid_x;
            tile->is_animating = false;
        }
    }
    if(tile->grid_y > tile->display_y) {
        tile->display_y += dist;
        if(tile->display_y >= tile->grid_y) {
            tile->display_y = tile->grid_y;
            tile->is_animating = false;
        }
    }
    else if (tile->grid_y < tile->display_y) {
        tile->display_y -= dist;
        if(tile->display_y <= tile->grid_y) {
            tile->display_y = tile->grid_y;
            tile->is_animating = false;
        }
    }
}
}

