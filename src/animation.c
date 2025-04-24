#include "animation.h"
#include "raylib.h"

#define ANIM_TIME 0.165f // In seconds.

float cubic_bezier(float t, float p0, float p1, float c0, float c1) {
    float t2 = t * t;
    float t3 = t2 * t;
    float mt = 1.0f - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    return mt3 * p0 + 3.0f * mt2 * t * c0 + 3.0f * mt * t2 * c1 + t3 * p1;
}

float smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

void animate_player(Player *player) {
  float delta = GetFrameTime();
  if (player->is_animating) {
    player->anim_timer += delta;
    float t = player->anim_timer / ANIM_TIME;
    if(t >= 1.0f) {
      t = 1.0f;
      player->is_animating = false;
    }
    // float bezier_factor_x = 0.01;
    // float bezier_factor_y = 0.01f;
    // float cx0 = player->start_x + bezier_factor_x;
    // float cx1 = player->grid_x - bezier_factor_x;
    // float cy0 = player->start_y + bezier_factor_y;
    // float cy1 = player->grid_y - bezier_factor_y;
    // player->display_x = cubic_bezier(t, player->start_x, player->grid_x, cx0, cx1);
    // player->display_y = cubic_bezier(t, player->start_y, player->grid_y, cy0, cy1);

    float st = smoothstep(t);
    player->display_x = player->start_x + (player->grid_x - player->start_x) * st;
    player->display_y = player->start_y + (player->grid_y - player->start_y) * st;

    if(!player->is_animating) {
      player->display_x = player->grid_x;
      player->display_y = player->grid_y;
      player->anim_timer = 0.0f;
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
