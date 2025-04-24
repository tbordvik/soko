#include <stddef.h>
#include <stdbool.h>

typedef struct {
    unsigned char content;
    int grid_x;
    int grid_y;
    float display_x;
    float display_y;
    bool is_animating;
    int animation_frame;
} Tile;

typedef struct {
    int grid_x, grid_y;
    float display_x, display_y;
    float start_x, start_y;
    float anim_timer;
    bool is_animating;
} Player;

/**/
/*bool is_empty(Stack *stack) {*/
/*    return stack->top == -1;*/
/*}*/
/**/
/*char pop(Stack *stack) {*/
/*    if(is_empty(stack)) {*/
/*        return NULL;*/
/*    }*/
/*    char popped = stack->key[stack->top];*/
/*    stack->top--;*/
/*    return popped;*/
/*}*/
/**/
/*void push(Stack *stack, char to_push) {*/
/*    if(stack->top == 31) {*/
/*        return;*/
/*    }*/
/*    stack->key[++stack->top] = to_push;*/
/*}*/
void animate_player(Player *player);
void animate_tile(Tile *tile);
