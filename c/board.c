#include "board.h"

Board * board_new(uint8_t size) {
    Board * self;

    self = (Board *)calloc(size * size + 3, sizeof(uint8_t));

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

    mem_size = (self->size * self->size + 3) * sizeof(uint8_t);
    clone = (Board *)malloc(mem_size);
    memcpy(clone, self, mem_size);
    return clone;
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
                else if(color == 1) fprintf(fp, "\xe2\x97\x8f ");
                else if(color == 2) fprintf(fp, "\xe2\x97\x8b ");
            }
        }
        fprintf(fp, "\n");
    }
}

int board_play(Board * self, uint8_t x, uint8_t y, uint8_t color) {
    uint8_t other_color, size, friends;
    uint32_t i, pos, own_libs;
    uint32_t killing, killed;

    other_color = 3 - color;
    size = self->size;
    pos = x * size + y;

    if(self->board[pos]) return 0;
    if(x == self->ko_x && y == self->ko_y) return 0;

    self->board[pos] = color;

    own_libs = _has_liberties(self, x, y, color);

    killing = 0;
    if(x > 0 && self->board[pos - size] == other_color && !_has_liberties(self, x - 1, y, other_color))
        killing |= 0x01;
    if(y > 0 && self->board[pos - 1] == other_color && !_has_liberties(self, x , y - 1, other_color))
        killing |= 0x02;
    if(x < size - 1 && self->board[pos + size] == other_color && !_has_liberties(self, x + 1, y, other_color))
        killing |= 0x04;
    if(y < size - 1 && self->board[pos + 1] == other_color && !_has_liberties(self, x, y + 1, other_color))
        killing |= 0x08;

    //fprintf(stdout, "(%d, %d) O: %d K: %d\n", x, y, own_libs, killing);

    if(!own_libs && !killing) {
        self->board[pos] = 0;
        return 0;
    }

    killed = 0;
    if(killing & 0x01) killed += _kill_group(self, x - 1, y, other_color);
    if(killing & 0x02) killed += _kill_group(self, x, y - 1, other_color);
    if(killing & 0x04) killed += _kill_group(self, x + 1, y, other_color);
    if(killing & 0x08) killed += _kill_group(self, x, y + 1, other_color);

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
    int mask, max_tonari, friends;
    uint8_t size;
    uint32_t pos;

    size = self->size;
    pos = x * size + y;

    max_tonari = 4;
    friends = 0;
    mask = 0xf;

    if(x <= 0) {max_tonari --; mask -= 0x1;}
    else if(self->board[pos - size] == color) friends ++;

    if(y <= 0) {max_tonari --; mask -= 0x2;}
    else if(self->board[pos - 1] == color) friends ++;

    if(x >= size - 1) {max_tonari --; mask -= 0x4;}
    else if(self->board[pos + size] == color) friends ++;

    if(y >= size - 1) {max_tonari --; mask -= 0x8;}
    else if(self->board[pos + 1] == color) friends ++;

    if (friends == max_tonari) return 1;
    // TODO: add diagonal data here;

    return 0;
}

uint32_t gcd(a, b) {
    uint32_t t;
    while(b) {
        t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int board_play_random(Board * self, uint8_t * x, uint8_t * y, uint8_t color) {
    uint32_t size, total, idx, step, count;

    size = self->size;
    total = size * size;
    idx = rand() % total;

    do step = (rand() % total) + 1;
    while (gcd(step, total) != 1);

    for (count = 0; count < total; count ++) {
        *x = idx / size;
        *y = idx % size;
        if(!_is_likely_eye(self, *x, *y, color)) {
            if(board_play(self, *x, *y, color)) return 1;
        }
        idx += step;
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

    //board_fprintf(stdout, c);
    //puts("");

    board_free(b);
    board_free(c);

    return 0;
}

int _play_rand() {
    Board * b;
    b = board_new(9);
    board_random_play_to_end(b, 1);
    board_fprintf(stdout, b);
    puts("");
    board_free(b);
}

int __BOARD_TEST() {
    uint64_t i;
    for(i = 0; i < 30; i++) {
        //_board_test();
        _play_rand();
    }
    return 0;
}
