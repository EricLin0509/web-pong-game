CC=clang
CC_WASM=emcc
CFLAGS=-Wall -Werror
EXE_CFLAGS= -lm -lSDL3 -lSDL3_ttf
WASM_CFLAGS= -Wno-experimental -sUSE_SDL=3 -sUSE_SDL_TTF=3 -sUSE_FREETYPE=1
EXPORTED_FUNCTIONS = -sEXPORTED_FUNCTIONS="['_main', '_start_game', '_pause_game', '_resume_game', '_restart_game', '_goto_menu', '_toggle_game_mode', '_toggle_player', '_set_snowflake_count']"
OUT=pong
SRC=paddle.c ball.c snow.c text.c main.c

all: release

wasm: $(SRC)
	$(CC_WASM) $(CFLAGS) $(WASM_CFLAGS) -O3 \
	-sENVIRONMENT=web \
	-sEXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setCanvasSize']" \
	$(EXPORTED_FUNCTIONS) \
	--preload-file roboto.ttf \
	$^ -o $(OUT).js

wasm_benchmark: $(SRC)
	$(CC_WASM) $(CFLAGS) $(WASM_CFLAGS) -O3 \
	-D BENCHMARK_MODE \
	-sENVIRONMENT=web \
	-sALLOW_MEMORY_GROWTH=1 \
	-sEXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setCanvasSize']" \
	$(EXPORTED_FUNCTIONS) \
	--preload-file roboto.ttf \
	--shell-file benchmark.html \
	$^ -o $(OUT).html

release: $(SRC)
	$(CC) $(CFLAGS) $(EXE_CFLAGS) -O3 $^ -o $(OUT)

debug: $(SRC)
	$(CC) $(CFLAGS) $(EXE_CFLAGS) -g $^ -o $(OUT)

run: debug
	./$(OUT)

clean:
	rm -f $(OUT)*

