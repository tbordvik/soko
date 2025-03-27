#include <stddef.h>
#include "raylib.h"

typedef struct {
    // Maybe 2-3 stored keystrokes is sufficient to store.. and if the user 
    // inputs more we overwrite the last one? So
    //
    // Hmm, try just storing one first and see how that goes? don't need a stack
    // for that tho..
    char key[32]; 
    int top;
} Stack;

typedef struct {
    unsigned char content;
    int grid_x;
    int grid_y;
    float display_x;
    float display_y;
    bool is_animating;
} Tile;

typedef struct {
    int x;
    int y;
    float displayX;
    float displayY;
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
