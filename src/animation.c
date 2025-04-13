#include "animation.h"
#include "raylib.h"

#define ANIM_TIME 0.215f // In seconds.

void animate_player(Player *player) {
  float delta = GetFrameTime();
  if (player->is_animating) {
    float dist = delta / ANIM_TIME;
    // Must be a better way to do this lol..
    // but to tired right now to simplify.
    if (player->x > player->display_x) {
      player->display_x += dist;
      if (player->display_x >= player->x) {
        player->display_x = player->x;
        player->is_animating = false;
      }
    } else if (player->x < player->display_x) {
      player->display_x -= dist;
      if (player->display_x <= player->x) {
        player->display_x = player->x;
        player->is_animating = false;
      }
    }
    if (player->y > player->display_y) {
      player->display_y += dist;
      if (player->display_y >= player->y) {
        player->display_y = player->y;
        player->is_animating = false;
      }
    } else if (player->y < player->display_y) {
      player->display_y -= dist;
      if (player->display_y <= player->y) {
        player->display_y = player->y;
        player->is_animating = false;
      }
    }
  }
}

void animate_tile(Tile *tile) {
  float delta = GetFrameTime();
  if (tile->is_animating) {
    float dist = delta / ANIM_TIME;
    // Must be a better way to do this lol..
    // but to tired right now to simplify.
    if (tile->grid_x > tile->display_x) {
      tile->display_x += dist;
      if (tile->display_x >= tile->grid_x) {
        tile->display_x = tile->grid_x;
        tile->is_animating = false;
      }
    } else if (tile->grid_x < tile->display_x) {
      tile->display_x -= dist;
      if (tile->display_x <= tile->grid_x) {
        tile->display_x = tile->grid_x;
        tile->is_animating = false;
      }
    }
    if (tile->grid_y > tile->display_y) {
      tile->display_y += dist;
      if (tile->display_y >= tile->grid_y) {
        tile->display_y = tile->grid_y;
        tile->is_animating = false;
      }
    } else if (tile->grid_y < tile->display_y) {
      tile->display_y -= dist;
      if (tile->display_y <= tile->grid_y) {
        tile->display_y = tile->grid_y;
        tile->is_animating = false;
      }
    }
  }
}
