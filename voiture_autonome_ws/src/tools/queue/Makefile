# Makefile de queue
CC := gcc
CFLAGS := -Wall -O2 -pthread $(INCLUDES)

# A modifier
SRC := queue.c

OBJ := $(SRC:%.c=$(BUILD_DIR)/%.o)


.PHONY: all


all: $(OBJ)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "✅ $< compilé"

clean:
	rm -rf $(BUILD_DIR)