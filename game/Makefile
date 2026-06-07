CC=clang
CC_WASM=emcc
CFLAGS=-Wall -Werror
EXE_CFLAGS= -lm -lSDL3 -lSDL3_ttf
WASM_CFLAGS= -Wno-experimental -sUSE_SDL=3 -sUSE_SDL_TTF=3 -sUSE_FREETYPE=1
EXPORTED_FUNCTIONS = -sEXPORTED_FUNCTIONS="['_main', '_start_game', '_pause_game', '_resume_game', '_restart_game', '_goto_menu', '_toggle_theme', '_toggle_game_mode', '_toggle_player', '_toggle_difficulty', '_set_snowflake_count']"
OUT=pong
SRC=paddle.c ball.c snow.c text.c main.c

all: release

gen_font:
	python gen-font-stream.py > roboto.h

wasm: gen_font
	$(CC_WASM) $(CFLAGS) $(WASM_CFLAGS) -O3 \
	-sENVIRONMENT=web \
	-sEXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setCanvasSize']" \
	$(EXPORTED_FUNCTIONS) \
	$(SRC) -o $(OUT).js

wasm_benchmark: gen_font
	$(CC_WASM) $(CFLAGS) $(WASM_CFLAGS) -O3 \
	-D BENCHMARK_MODE \
	-sENVIRONMENT=web \
	-sALLOW_MEMORY_GROWTH=1 \
	-sEXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setCanvasSize']" \
	$(EXPORTED_FUNCTIONS) \
	--shell-file benchmark.html \
	$(SRC) -o $(OUT).html

release: gen_font
	$(CC) $(CFLAGS) $(EXE_CFLAGS) -O3 $(SRC) -o $(OUT)

debug: gen_font
	$(CC) $(CFLAGS) $(EXE_CFLAGS) -g $(SRC) -o $(OUT)

run: debug
	./$(OUT)

install: release
	install -m 755 $(OUT) /usr/bin/$(OUT)

uninstall:
	rm -f /usr/bin/$(OUT)

clean:
	rm -f $(OUT)*

