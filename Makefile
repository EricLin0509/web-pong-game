CC=clang
CFLAGS=-Wall -Wextra -Werror -lSDL3 -lSDL3_ttf
OUT=pong
SRC=main.c

all: release

release: $(SRC)
	$(CC) $(CFLAGS) -O3 $^ -o $(OUT)

debug: $(SRC)
	$(CC) $(CFLAGS) -g $^ -o $(OUT)

run: debug
	./$(OUT)

clean:
	rm -f $(OUT)

