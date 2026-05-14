CC=clang
CC_WASM=emcc
CFLAGS=-Wall -Werror
EXE_CFLAGS= -lSDL3 -lSDL3_ttf
WASM_CFLAGS= -Wno-experimental -sUSE_SDL=3 --use-port=sdl3_ttf
OUT=pong
SRC=main.c

all: release

wasm: $(SRC)
	$(CC_WASM) $(CFLAGS) $(WASM_CFLAGS) -O3 $^ -o $(OUT).wasm

release: $(SRC)
	$(CC) $(CFLAGS) $(EXE_CFLAGS) -O3 $^ -o $(OUT)

debug: $(SRC)
	$(CC) $(CFLAGS) $(EXE_CFLAGS) -g $^ -o $(OUT)

run: debug
	./$(OUT)

clean:
	rm -f $(OUT)*

