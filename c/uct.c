#include <math.h>

#include "board.h"
#include "uct.h"

UctNode * uct_node_new(UctNode * parent) {

    UctNode * self;

    self = (UctNode *)calloc(1, sizeof(struct UctNode));
    self->parent = parent;

    //fprintf(stderr, "ALLOC: %p\n", self);

    return self;
}

UctNode * uct_node_select_child(UctNode * self) {
    UctNode * chosen, * curr_child;
    double val, best_val;

    best_val = 0;
    chosen = self->children;
    curr_child = self->children;

    while(1) {
        val = ((double)(curr_child->wins)) / curr_child->visits +
            (UCT_K * sqrt(2.0 * log((double)(self->visits)) / curr_child->visits));

        if(val > best_val) {
            chosen = curr_child;
            best_val = val;
        }
        if(!curr_child->sibling) break;
        curr_child = curr_child->sibling;
    }

    return chosen;
}

UctNode * uct_node_add_child(UctNode * self, uint8_t x, uint8_t y, uint8_t color) {
    UctNode * child;

    child = self->children;
    while(child){
        if(child->x == x && child->y == y) return child;
        child = child->sibling;
    }

    child = uct_node_new(self);
    child->x = x;
    child->y = y;
    child->color = color;
    child->sibling = self->children;

    self->children = child;

    return child;
}

int _uct_node_fprintf_recur(FILE * fp, UctNode * self, int indent) {
    UctNode * child;
    int i;

    for (i = 0; i < indent; i++) fprintf(fp, "  ");
    fprintf(fp, "%d:(%d, %d) W: %d V:%d\n", self->color, self->x, self->y,
        self->wins, self->visits);

    child = self->children;
    while(child) {
        _uct_node_fprintf_recur(fp, child, indent + 1);
        child = child->sibling;
    }
    return 1;
}

int uct_node_fprintf(FILE * fp, UctNode * self) {
    _uct_node_fprintf_recur(fp, self, 0);
}

int uct_node_free(UctNode * self) {
    if(self->children) uct_node_free(self->children);
    if(self->sibling) uct_node_free(self->sibling);
    free(self);
    return 1;
}

uint32_t uct_eval(Board * board, uint32_t iterations, uint8_t *x, uint8_t *y, uint8_t color) {

    UctNode * root, * node, *best_child;
    Board * board_copy;
    uint8_t new_move, i, j, curr_color;
    int32_t score, most_visits;

    root = uct_node_new(0);

    while(iterations) {
        node = root;
        board_copy = board_clone(board);
        curr_color = color;

        while(rand() % 1000 > UCT_EXPLORE && node->children) {
            node = uct_node_select_child(node);
            //printf("SELECTED: %d=%d (%d, %d)\n",
            //    node->color, curr_color, node->x, node->y);
            board_play(board_copy, node->x, node->y, curr_color);
            curr_color = 3 - curr_color;
        }

        new_move = board_play_random(board_copy, &i, &j, curr_color);

        //puts("\x1b[2J");
        //board_fprintf(stdout, board_copy);
        //puts("");

        if(new_move) {
            node = uct_node_add_child(node, i, j, curr_color);
            curr_color = 3 - curr_color;
        }

        board_random_play_to_end(board_copy, curr_color);
        score = board_score(board_copy);

        while(node) {
            node->visits ++;
            if(node->color == 1 && score < 0 || node->color == 2 && score > 0)
                node->wins ++;
            node = node->parent;
        }

        board_free(board_copy);
        iterations --;
        //puts("\x1b[2J");
        //uct_node_fprintf(stdout, root);
    }

    if( root->children ) {
        most_visits = 0;
        node = root->children;
        while(node) {
            if(node->visits > most_visits) {
                most_visits = node->visits;
                best_child = node;
            }
            node = node->sibling;
        }
        *x = best_child->x;
        *y = best_child->y;
        printf("ODDS %d: %.1f%% (%d/%d)\n", color,
            100.0 * best_child->wins / best_child->visits,
            best_child->wins, best_child->visits);
    }
    uct_node_free(root);
}

int __UCT_TEST(){
    uint8_t x, y, color;
    Board * board;
    int i, j;

    board = board_new(9);

    color = 1;

    while(1) {
        //puts("\x1b[2J");
        board_fprintf(stdout, board);
        puts("");
        uct_eval(board, 70000, &x, &y, 1);
        board_play(board, x, y, 1);

        board_fprintf(stdout, board);
        puts("");

        uct_eval(board, 70000, &x, &y, 2);
        board_play(board, x, y, 2);

        //printf("WHITE: ");
        //scanf("%d%d", &i, &j);
        //board_play(board, (uint8_t)i, (uint8_t)j, 2);
    }

    board_free(board);

    printf("hello world!");

    return 0;
}
