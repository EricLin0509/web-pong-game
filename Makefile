GAMEDIR=game
MODULES=pong.wasm pong.js pong.data
VUE_DIR=web-pong-game
VUE_WASM_DIR=$(VUE_DIR)/public/wasm

all: build_game copy_files

build_game:
	make -C $(GAMEDIR) wasm

copy_files:
	mkdir -p $(VUE_WASM_DIR)
	for file in $(MODULES); do \
		cp $(GAMEDIR)/$$file $(VUE_WASM_DIR)/$$file; \
	done

run_server:
	make -C $(VUE_DIR) run

clean:
	make -C $(GAMEDIR) clean
	rm -rf $(VUE_WASM_DIR)/*
