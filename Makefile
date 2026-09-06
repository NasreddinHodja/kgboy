CC = gcc

SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS   := $(shell pkg-config --libs sdl2)

CFLAGS = -Wall -Wextra -std=c17 -g -Iinclude -MMD -MP $(SDL_CFLAGS)
LDLIBS = $(SDL_LIBS)

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP = $(OBJ:.o=.d)
TARGET = $(BIN_DIR)/gbemu

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEP)

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@


ROM_ASM = $(wildcard roms/*.asm)
ROMS    = $(ROM_ASM:.asm=.gb)

roms: $(ROMS)

roms/%.gb: roms/%.asm
	rgbasm -o roms/$*.o $<
	rgblink -o $@ roms/$*.o
	rgbfix -v -p 0xFF $@
	rm -f roms/$*.o

.PHONY: roms

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: clean
