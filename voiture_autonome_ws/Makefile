# ==============================
# Makefile global
# ==============================

CC := gcc
ROOT_DIR := $(abspath .)
SRC_DIR := $(ROOT_DIR)/src
BUILD_DIR := $(ROOT_DIR)/build

# === Includes ===
INCLUDES_TOOLS :=  $(addprefix -I,$(wildcard $(SRC_DIR)/tools/*/))
INCLUDES_COMMON := -I$(SRC_DIR)/common
CFLAGS := -Wall -O2 -pthread -lm

# Liste des modules à ignorer : ex. make voiture DISABLE=Localisation,Comportement
DISABLE ?=

# ==============================
# Règles principales
# ==============================
all: tools common voiture controleur

# ========= TOOLS =========
tools:
	@mkdir -p $(BUILD_DIR)/tools
	@$(MAKE) --no-print-directory -C $(SRC_DIR)/tools BUILD_DIR=$(BUILD_DIR)/tools INCLUDES_TOOLS="$(INCLUDES_TOOLS)"


# ========= COMMON =========
COMMON_DIR := $(SRC_DIR)/common
COMMON_BUILD := $(BUILD_DIR)/common
COMMON_SRCS := $(wildcard $(COMMON_DIR)/*.c)
COMMON_OBJS := $(patsubst $(COMMON_DIR)/%.c,$(COMMON_BUILD)/%.o,$(COMMON_SRCS))

common: $(COMMON_OBJS)
$(COMMON_BUILD)/%.o: $(COMMON_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES_TOOLS) $(INCLUDES_COMMON) -c $< -o $@

# ========= VOITURE =========
VOITURE_DIR := $(SRC_DIR)/voiture
VOITURE_BUILD := $(BUILD_DIR)/voiture
VOITURE_EXEC := $(VOITURE_BUILD)/voiture

voiture:
	@echo "🚗 Compilation du projet voiture..."
	@mkdir -p $(VOITURE_BUILD)
	@$(MAKE) --no-print-directory tools
	@$(MAKE) --no-print-directory common
	@$(MAKE) --no-print-directory -C $(VOITURE_DIR) \
		VOITURE_BUILD=$(VOITURE_BUILD) \
		COMMON_INCLUDES="$(INCLUDES_COMMON)" \
		TOOLS_INCLUDES="$(INCLUDES_TOOLS)" \
		DISABLE="$(DISABLE)" \
		all
	@$(MAKE) --no-print-directory voiture-link

# ========= LINKAGE =========
voiture-link:
	@echo "🔗 Edition des liens pour voiture..."
	@mkdir -p $(VOITURE_BUILD)
	@OBJS="$$(find $(VOITURE_BUILD) -name '*.o') $$(find $(BUILD_DIR)/tools -name '*.o') $$(find $(BUILD_DIR)/common -name '*.o')"; \
	echo -n "📄 Objets à linker : "; \
	for f in $$OBJS; do echo -n "$$(basename $$f) "; done; \
	echo; \
	$(CC) $(CFLAGS) $$OBJS -o $(VOITURE_EXEC) && \
	echo "✔ Exécutable généré : $(VOITURE_EXEC)";


# ========= CONTROLEUR =========
CONTROLEUR_DIR := $(SRC_DIR)/controleur
CONTROLEUR_BUILD := $(BUILD_DIR)/controleur
CONTROLEUR_EXEC := $(CONTROLEUR_BUILD)/controleur

controleur:
	@echo "🎮 Compilation du projet controleur..."
	@mkdir -p $(CONTROLEUR_BUILD)
	@$(MAKE) --no-print-directory tools
	@$(MAKE) --no-print-directory common
	@$(MAKE) --no-print-directory -C $(CONTROLEUR_DIR) \
		CONTROLEUR_BUILD=$(CONTROLEUR_BUILD) \
		COMMON_INCLUDES="$(INCLUDES_COMMON)" \
		TOOLS_INCLUDES="$(INCLUDES_TOOLS)" \
		DISABLE="$(DISABLE)" \
		all
	@$(MAKE) --no-print-directory controleur-link

# ========= LINKAGE =========
controleur-link:
	@echo "\n🔗 Edition des liens pour controleur..."
	@mkdir -p $(CONTROLEUR_BUILD)
	@OBJS="$$(find $(CONTROLEUR_BUILD) -name '*.o') $$(find $(BUILD_DIR)/tools -name '*.o') $$(find $(BUILD_DIR)/common -name '*.o')"; \
	echo -n "📄 Objets à linker : "; \
	for f in $$OBJS; do echo -n "$$(basename $$f) "; done; \
	echo; \
	$(CC) $(CFLAGS) $$OBJS -o $(CONTROLEUR_EXEC) && \
	echo "✔ Exécutable généré : $(CONTROLEUR_EXEC)"



# ========= NETTOYAGE =========
clean:
	rm -rf $(BUILD_DIR)
	@echo "🧹 Nettoyage terminé"

# ========= AIDE =========
help:
	@echo "================= 📘 AIDE MAKEFILE ================="
	@echo "Commandes disponibles :"
	@echo "  make all           → Compile l'ensemble du workspace (tools, common, voiture, controleur)"
	@echo "  make tools         → Compile les outils (src/tools)"
	@echo "  make common        → Compile les fichiers communs (src/common)"
	@echo "  make voiture       → Compile et lie le projet voiture"
	@echo "  make controleur    → Compile et lie le projet controleur"
	@echo "  make clean         → Supprime tous les fichiers compilés (build/)"
	@echo ""
	@echo "Options :"
	@echo "  DISABLE=<modules>  → Désactive certains modules lors de la compilation"
	@echo "                       Exemple : make voiture DISABLE=Localisation,Comportement"
	@echo "===================================================="



# ========= CIBLE PAR DÉFAUT =========
.DEFAULT:
	@echo "❌ Cible inconnue : '$@'"
	@echo "Utilisez 'make help' pour la liste des commandes disponibles."
	@exit 1