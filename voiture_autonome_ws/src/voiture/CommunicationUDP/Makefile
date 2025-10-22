# ==============================
# Makefile module : CommunicationUDP
# ==============================

CC := gcc

ARGS := $(CFLAGS) $(INCLUDES)     # Flags de compilation
SRC := UDP_voiture.c               # Seulement les fichiers source
LIBS := -lcjson                    # Bibliothèques à lier

# Création de la liste des fichiers objets à créer (.o)
OBJS := $(addprefix $(BUILD_DIR)/, $(SRC:.c=.o))

all: $(OBJS)

# Compilation des fichiers .c en .o
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@) 
	$(CC) $(ARGS) -c $< -o $@ 
	@echo "✅ Module CommunicationUDP : $@ compilé"

# Edition de lien (à faire quand tu construis l’exécutable)
voiture: $(OBJS)
	$(CC) $(OBJS) -o voiture $(LIBS)
