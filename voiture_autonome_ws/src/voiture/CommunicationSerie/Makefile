# ==============================
# Makefile module : CommunicationSerie
# ==============================

# BUILD_DIR est le chemin absolue vers le dossier de build, typiquement ws/build/voiture/*/
# INCLUDES contient les arguments -I<path> des dossiers src/tools/*/, src/common/, src/voiture/ et src/voiture/*/

CC := gcc

# Partie à modifier 
# ==============================
ARGS := $(CFLAGS) $(INCLUDES)     # Possibilité d'ajouter des flags (-Wall -O2 -lm -pthread par défaut)
SRC := comm_serie_utils.c communication_serie.c      # A modifier lorsqu'on ajoute des fichiers de code
# ==============================

# Création de la liste des fichiers objets à créer (.o)
OBJS := $(addprefix $(BUILD_DIR)/, $(SRC:.c=.o))

all: $(OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@) 
	$(CC) $(ARGS) -c $< -o $@ 
	@echo "✅ Module CommunicationSerie : $@ compilé" 
