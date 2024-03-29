#ifndef BOARD_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define BOARD_MASK_BLACK    0x01
#define BOARD_MASK_WHITE    0x02
#define BOARD_MASK_MARK     0x04

typedef struct Board {
    uint32_t captures[2]; // size = 8 * uint8_t
    uint8_t size;
    uint8_t ko_x;
    uint8_t ko_y;
    uint8_t board[];
} Board;

Board * board_new(uint8_t size);

Board * board_clone(Board * self);

int board_free(Board * self);

int board_play(Board * self, uint8_t x, uint8_t y, uint8_t color);

int board_play_random(Board * self, uint8_t * x, uint8_t * y, uint8_t color);

int board_random_play_to_end(Board * self, uint8_t color);

float board_score(Board * self);

int board_fprintf(FILE * fp, Board * self);


#define BOARD_H
#endif
