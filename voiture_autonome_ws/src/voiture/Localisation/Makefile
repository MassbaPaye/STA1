# ==============================
# Makefile module : Localisation
# ==============================

CC := gcc
CFLAGS := -Wall -O2 $(INCLUDES)

SRC := main_localisation.c
OBJS := $(addprefix $(BUILD_DIR)/, $(SRC:.c=.o))

all: $(OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "✅ Module Localisation : $@ compilé"
