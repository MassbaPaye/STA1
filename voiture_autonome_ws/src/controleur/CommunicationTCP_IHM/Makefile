# ==============================
# Makefile module : CommunicationTCP IHM
# ==============================

# BUILD_DIR est le chemin absolue vers le dossier de build, typiquement ws/build/controleur/ModuleExemple/
# INCLUDES contient les arguments -I<path> des dossiers src/tools/*/, src/common/, src/controleur/ et src/controleur/*/

CC := gcc
# gcc TCP_controleur.c message_controleur.c -o controleur -lpthread
# Partie à modifier 
# ==============================
ARGS := $(CFLAGS) $(INCLUDES)     # Possibilité d'ajouter des flags (-Wall -O2 -lm -pthread par défaut)
SRC := communication_tcp_ihm.c    # A modifier lorsqu'on ajoute des fichiers de code
# ==============================

# Création de la liste des fichiers objets à créer (.o)
OBJS := $(addprefix $(BUILD_DIR)/, $(SRC:.c=.o))

all: $(OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@) 
	$(CC) $(ARGS) -c $< -o $@ 
	@echo "✅ Module CommunicationTCP_IHM : $@ compilé" 
