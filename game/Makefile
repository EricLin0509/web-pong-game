CC=clang
CC_WASM=emcc
CFLAGS=-Wall -Werror
EXE_CFLAGS= -lSDL3 -lSDL3_ttf
WASM_CFLAGS= -Wno-experimental -sUSE_SDL=3 -sUSE_SDL_TTF=3 -sUSE_FREETYPE=1
OUT=pong
SRC=main.c

all: release

wasm: $(SRC)
	$(CC_WASM) $(CFLAGS) $(WASM_CFLAGS) -O3 \
	-sENVIRONMENT=web \
	-sEXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setCanvasSize']" \
	-sEXPORTED_FUNCTIONS="['_main', '_pause_game']" \
	--preload-file roboto.ttf \
	--shell-file shell.html \
	$^ -o $(OUT).html

release: $(SRC)
	$(CC) $(CFLAGS) $(EXE_CFLAGS) -O3 $^ -o $(OUT)

debug: $(SRC)
	$(CC) $(CFLAGS) $(EXE_CFLAGS) -g $^ -o $(OUT)

run: debug
	./$(OUT)

clean:
	rm -f $(OUT)*

