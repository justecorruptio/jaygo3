#include "board.h"

Board * board_new(uint8_t size) {
    Board * self;

    self = (Board *)calloc(size * size + 3 + 8, sizeof(uint8_t));

    self->size = size;
    self->ko_x = size;
    self->ko_y = size;

    return self;
}

int board_free(Board * self) {
    free(self);
}

Board * board_clone(Board * self) {
    Board * clone;
    size_t mem_size;

    mem_size = (self->size * self->size + 3 + 8) * sizeof(uint8_t);
    clone = (Board *)malloc(mem_size);
    memcpy(clone, self, mem_size);
    return clone;
}

uint32_t _has_liberties_recur(Board * self, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t pos_color, size;
    uint32_t pos, has_libs;

    size = self->size;

    pos = x * size + y;

    pos_color = self->board[pos];

    if(pos_color == 3 - color) return 0;
    if(pos_color == 0) return 1;

    self->board[pos] |= BOARD_MASK_MARK;

    has_libs =
        x > 0 && (
            self->board[pos - size] == 0 ||
            self->board[pos - size] == color && _has_liberties_recur(self, x - 1, y, color)
        ) ||
        y > 0 && (
            self->board[pos - 1] == 0 ||
            self->board[pos - 1] == color && _has_liberties_recur(self, x , y - 1, color)
        ) ||
        x < size - 1 && (
            self->board[pos + size] == 0 ||
            self->board[pos + size] == color && _has_liberties_recur(self, x + 1, y, color)
        ) ||
        y < size - 1 && (
            self->board[pos + 1] == 0 ||
            self->board[pos + 1] == color && _has_liberties_recur(self, x, y + 1, color)
        );

    //self->board[pos] -= BOARD_MASK_MARK;

    return has_libs;
}

int _clear_marks(Board * self, uint8_t x, uint8_t y) {
    uint8_t size;
    uint32_t pos, has_libs;

    size = self->size;
    pos = x * size + y;

    self->board[pos] -= BOARD_MASK_MARK;

    if(x > 0 && (self->board[pos - size] & BOARD_MASK_MARK)) _clear_marks(self, x - 1, y);
    if(y > 0 && (self->board[pos - 1] & BOARD_MASK_MARK)) _clear_marks(self, x, y - 1);
    if(x < size - 1 && (self->board[pos + size] & BOARD_MASK_MARK)) _clear_marks(self, x + 1, y);
    if(y < size - 1 && (self->board[pos + 1] & BOARD_MASK_MARK)) _clear_marks(self, x, y + 1);

    return 0;
}

uint32_t _has_liberties(Board * self, uint8_t x, uint8_t y, uint8_t color) {
    uint32_t has_libs;

    has_libs = _has_liberties_recur(self, x, y, color);
    _clear_marks(self, x, y);
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

    killed = 1;

    if(x > 0) killed += _kill_group(self, x - 1, y, color);
    if(y > 0) killed += _kill_group(self, x , y - 1, color);
    if(x < size - 1) killed += _kill_group(self, x + 1, y, color);
    if(y < size - 1) killed += _kill_group(self, x, y + 1, color);

    return killed;
}

int board_fprintf(FILE * fp, Board * self) {
    uint8_t i, j, color;
    for(i = 0; i < self->size; i ++) {
        for(j = 0; j < self->size; j++) {
            if(i == self->ko_x && j == self->ko_y)
                fprintf(fp, "# ");
            else {
                //fprintf(fp, "%c ", ".XO"[self->board[i * self->size + j]]);
                color = self->board[i * self->size + j];
                if(color == 0) fprintf(fp, ". ");
                else if(color == 1) fprintf(fp, "\xe2\x97\x8b ");
                else if(color == 2) fprintf(fp, "\xe2\x97\x8f ");
            }
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "CAP - B: %d, W: %d\n", self->captures[0], self->captures[1]);
}

int board_play(Board * self, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t other_color, size, friends;
    uint32_t i, pos, own_libs;
    uint32_t killing, killed;

    int a, b;

    other_color = 3 - color;
    size = self->size;

    // pass
    if(x == size && y == size) {
        self->ko_x = size;
        self->ko_y = size;
        return 1;
    }

    pos = x * size + y;

    if(self->board[pos]) return 0;
    if(x == self->ko_x && y == self->ko_y) return 0;

    self->board[pos] = color;

    own_libs = _has_liberties(self, x, y, color);

    /*
    for(a = 0; a<size;a++){
        for(b=0;b<size;b++)
            fprintf(stdout, "%x ", self->board[a * size + b]);
        puts("");
    }
    puts("");
    */

    killing = 0;
    if(x > 0 && self->board[pos - size] == other_color && !_has_liberties(self, x - 1, y, other_color))
        killing |= 0x01;
    if(y > 0 && self->board[pos - 1] == other_color && !_has_liberties(self, x , y - 1, other_color))
        killing |= 0x02;
    if(x < size - 1 && self->board[pos + size] == other_color && !_has_liberties(self, x + 1, y, other_color))
        killing |= 0x04;
    if(y < size - 1 && self->board[pos + 1] == other_color && !_has_liberties(self, x, y + 1, other_color))
        killing |= 0x08;


    //fprintf(stderr, "(%d, %d) O: %d K: %d\n", x, y, own_libs, killing);

    if(!own_libs && !killing) {
        self->board[pos] = 0;
        return 0;
    }

    killed = 0;
    if(killing & 0x01) killed += _kill_group(self, x - 1, y, other_color);
    if(killing & 0x02) killed += _kill_group(self, x, y - 1, other_color);
    if(killing & 0x04) killed += _kill_group(self, x + 1, y, other_color);
    if(killing & 0x08) killed += _kill_group(self, x, y + 1, other_color);

    self->captures[color - 1] += killed;

    self->ko_x = size;
    self->ko_y = size;
    if(killed == 1) {
        friends =
            (x > 0 && self->board[pos - size] == color) ||
            (y > 0 && self->board[pos - 1] == color) ||
            (x < size - 1 && self->board[pos + size] == color) ||
            (y < size - 1 && self->board[pos + 1] == color);

        if(!friends) {
            if(killing & 0x01)      {self->ko_x = x - 1; self->ko_y = y;}
            else if(killing & 0x02) {self->ko_x = x; self->ko_y = y - 1;}
            else if(killing & 0x04) {self->ko_x = x + 1; self->ko_y = y;}
            else if(killing & 0x08) {self->ko_x = x; self->ko_y = y + 1;}
        }
    }

    return 1;
}

int _is_likely_eye(Board * self, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t size, mask, diags;
    uint32_t pos;

    size = self->size;
    pos = x * size + y;

    mask = 0xf;

    if(x <= 0) {mask -= 0x1;}
    else if(self->board[pos - size] != color) return 0;

    if(y <= 0) {mask -= 0x2;}
    else if(self->board[pos - 1] != color) return 0;

    if(x >= size - 1) {mask -= 0x4;}
    else if(self->board[pos + size] != color) return 0;

    if(y >= size - 1) {mask -= 0x8;}
    else if(self->board[pos + 1] != color) return 0;

    //diags = 0;

    //if(((mask & 0x3) == 0x3) && self->board[pos - size - 1] == color) diags ++;
    //if(((mask & 0x6) == 0x6) && self->board[pos + size - 1] == color) diags ++;
    //if(((mask & 0xc) == 0xc) && self->board[pos + size + 1] == color) diags ++;
    //if(((mask & 0x9) == 0x9) && self->board[pos - size + 1] == color) diags ++;

    //if (diags >= 1) return 1;

    return 1;
}

int board_play_random(Board * self, uint8_t * x, uint8_t * y, uint8_t color) {
    uint32_t size, total, idx, count;

    size = self->size;
    total = size * size;
    idx = rand() % total;

    for (count = 0; count < total; count ++) {
        *x = idx / size;
        *y = idx % size;
        if(!_is_likely_eye(self, *x, *y, color)) {
            if(board_play(self, *x, *y, color)) return 1;
        }
        idx += 1;
        if (idx >= total) idx -= total;
    }
    return 0;
}

int board_random_play_to_end(Board * self, uint8_t color) {
    int played, passed, turn, max_turns;
    uint8_t x, y;

    turn = color;
    passed = 0;

    // prevent superkos
    max_turns = self->size * self->size * 2;

    while (passed < 2 && max_turns) {
        played = board_play_random(self, &x, &y, turn);
        if(played) passed = 0;
        else passed += 1;
        if(!played) {
            // passing
            board_play(self, self->size, self->size, turn);
        }

        //if(played) fprintf(stdout, "%d: (%d, %d)\n", turn, x, y);
        //else fprintf(stdout, "%d: passed\n", turn);

        //puts("\x1b[2J");
        //board_fprintf(stdout, self);
        //puts("");
        //system("sleep .001");

        turn = 3 - turn;
        max_turns --;
    }

}

int _board_test() {
    Board * b;
    Board * c;

    b = board_new(9);

    board_play(b, 0, 1, 2);
    board_play(b, 1, 0, 2);
    board_play(b, 1, 1, 2);

    board_play(b, 0, 2, 1);
    board_play(b, 1, 2, 1);
    board_play(b, 2, 1, 1);
    board_play(b, 2, 0, 1);

    board_play(b, 0, 0, 1);

    c = board_clone(b);

    board_play(c, 0, 1, 2);
    board_play(c, 1, 0, 2);

    board_fprintf(stdout, c);
    puts("");

    board_free(b);
    board_free(c);

    return 0;
}

int _play_rand() {
    Board * b;
    b = board_new(9);
    board_random_play_to_end(b, 1);
    //puts("\x1b[2J");
    //board_fprintf(stdout, b);
    //puts("");
    board_free(b);
}

int __BOARD_TEST() {
    uint64_t i;
    for(i = 0; i < 10000; i++) {
        //_board_test();
        _play_rand();
    }
    return 0;
}
