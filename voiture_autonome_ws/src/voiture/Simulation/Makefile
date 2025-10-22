# ==============================
# Makefile module : Simulation
# ==============================

CC := gcc

# Partie à modifier 
# ==============================
ARGS := $(CFLAGS) $(INCLUDES)  # Possibilité d'ajouter des flags (-lm, -pthread par exemple)
SRC := simulation_loc.c # A modifier lorsqu'on ajoute des fichiers de code
# ==============================

# Création de la liste des fichiers objets à créer (.o)
OBJS := $(addprefix $(BUILD_DIR)/, $(SRC:.c=.o))

all: $(OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@) 
	@$(CC) $(ARGS) -c $< -o $@ 
	@echo "✅ Module Simulation : $@ compilé"
