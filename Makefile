CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -g -Iinclude -MMD -MP

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP = $(OBJ:.o=.d)
TARGET = $(BIN_DIR)/gbemu

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEP)

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: clean
