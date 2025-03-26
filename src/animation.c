#include "raylib.h"
#include "animation.h"

// Calculate the current animation frame
void animate_player(Player *player) {
float delta = GetFrameTime();
float animation_time = 0.2f;
if(player->is_animating) {
    float dist = delta / animation_time;
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

