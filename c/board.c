#include "board.h"

Board * board_new(uint8_t size) {
    Board * self;

    self = (Board *)calloc(size * size + 1, sizeof(uint8_t));

    self->size = size;
    return self;

}

int board_free(Board * self) {
    free(self);
}

uint32_t _has_liberties(Board * self, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t pos_color, size;
    uint32_t pos, has_libs;

    size = self->size;

    pos = x * size + y;

    if(self->board[pos] & BOARD_MASK_MARK) return 0;

    pos_color = self->board[pos] & 0x03;

    if(pos_color == 3 - color) return 0;
    if(pos_color == 0) return 1;


    self->board[pos] |= BOARD_MASK_MARK;

    has_libs =
        x > 0 && ((self->board[pos - size] & 0x03) == 0 || _has_liberties(self, x - 1, y, color)) ||
        y > 0 && ((self->board[pos - 1] & 0x03) == 0 || _has_liberties(self, x , y - 1, color)) ||
        x < size - 1 && ((self->board[pos + size] & 0x03) == 0 || _has_liberties(self, x + 1, y, color)) ||
        y < size - 1 && ((self->board[pos + 1] & 0x03) == 0 || _has_liberties(self, x, y + 1, color));

    self->board[pos] -= BOARD_MASK_MARK;

    return has_libs;
}

uint32_t _kill_group(Board * self, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t pos_color, size;
    uint32_t pos, killed;

    size = self->size;

    pos = x * size + y;
    pos_color = self->board[pos] & 0x03;

    if(pos_color != color) return 0;

    self->board[pos] = 0;

    killed = 0;

    if(x > 0) killed += _kill_group(self, x - 1, y, color);
    if(y > 0) killed += _kill_group(self, x , y - 1, color);
    if(x < size - 1) killed += _kill_group(self, x + 1, y, color);
    if(y < size - 1) killed += _kill_group(self, x, y + 1, color);

    return killed;
}

int board_fprintf(FILE * fp, Board * self) {
    uint8_t i, j;
    for(i = 0; i < self->size; i ++) {
        for(j = 0; j < self->size; j++) {
            fprintf(fp, "%c ", ".XO"[self->board[i * self->size + j]]);
        }
        fprintf(fp, "\n");
    }
}

int board_play(Board * self, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t other_color;
    uint32_t i, pos, own_libs;
    uint32_t killing, killed; //N, S, E, W

    other_color = 3 - color;

    pos = x * self->size + y;

    if(self->board[pos]) return 0;

    self->board[pos] = color;

    own_libs = _has_liberties(self, x, y, color);

    killing = 0;
    if(x > 0 && !_has_liberties(self, x - 1, y, other_color)) killing |= 0x01;
    if(y > 0 && !_has_liberties(self, x , y - 1, other_color)) killing |= 0x02;
    if(x < self->size - 1 && !_has_liberties(self, x + 1, y, other_color)) killing |= 0x04;
    if(y < self->size - 1 && !_has_liberties(self, x, y + 1, other_color)) killing |= 0x08;

    if(!own_libs && !killing) {
        self->board[pos] = 0;
        return 0;
    }

    killed = 0;
    if(killing & 0x01) killed += _kill_group(self, x - 1, y, other_color);
    if(killing & 0x02) killed += _kill_group(self, x, y - 1, other_color);
    if(killing & 0x04) killed += _kill_group(self, x + 1, y, other_color);
    if(killing & 0x08) killed += _kill_group(self, x, y + 1, other_color);


}

int _board_test() {
    Board * b;

    b = board_new(9);

    board_play(b, 0, 1, 2);
    board_play(b, 1, 0, 2);
    board_play(b, 1, 1, 2);

    board_play(b, 0, 2, 1);
    board_play(b, 1, 2, 1);
    board_play(b, 2, 1, 1);
    board_play(b, 2, 0, 1);

    board_play(b, 0, 0, 1);

    board_play(b, 0, 1, 2);
    board_play(b, 1, 0, 2);

    //board_fprintf(stdout, b);
    //puts("");

    board_free(b);

    return 0;
}

int __BOARD_TEST() {
    uint64_t i;
    for(i = 0; i < 1800000; i++) {
        _board_test();
    }
    return 0;
}
