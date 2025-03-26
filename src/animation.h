#include "raylib.h"

typedef struct {
    int x;
    int y;
    float displayX;
    float displayY;
    bool is_animating;
} Player;

void animate_player(Player *player);
