#ifndef UCT_H
#define UCT_H

#define UCT_EXPLORE 15
#define UCT_K 1

typedef struct UctNode {
    struct UctNode * parent;
    struct UctNode * children;
    struct UctNode * sibling;
    uint32_t wins;
    uint32_t visits;
    uint32_t num_children;

    uint8_t x, y;
    uint8_t color;

} UctNode;

#endif
