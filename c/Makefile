CC=gcc -O3

all: jaygo

jaygo: main.c set.c
	$(CC) main.c set.c -o jaygo

test_set: set.c
	$(CC) set.c -o test -D__SET_TEST=main -DTESTING && ./test && rm ./test

test_board: board.c
	$(CC) board.c -o test -D__BOARD_TEST=main -DTESTING && time ./test #&& rm ./test

clean:
	rm jaygo
