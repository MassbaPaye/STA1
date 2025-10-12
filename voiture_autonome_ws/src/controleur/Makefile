# ==============================
# Makefile pour src/controleur
# ==============================

# Compilateur
CC := gcc
CFLAGS := -Wall -O2 -pthread


# Variables pour le build
CONTROLEUR_DIR := $(CURDIR)
CONTROLEUR_BUILD ?= $(CURDIR)/build


# Liste des sous-dossiers (= modules) avec chemin complet
CONTROLEUR_MODULES := $(wildcard $(CONTROLEUR_DIR)/*/)

INCLUDES = $(COMMON_INCLUDES) $(TOOLS_INCLUDES) -I$(CONTROLEUR_DIR) $(addprefix -I, $(CONTROLEUR_MODULES))

# Fichiers .c dans CONTROLEUR_DIR (hors sous-dossiers)
CONTROLEUR_C_FILES := $(wildcard $(CONTROLEUR_DIR)/*.c)
CONTROLEUR_OBJS := $(patsubst $(CONTROLEUR_DIR)/%.c,$(CONTROLEUR_BUILD)/%.o,$(CONTROLEUR_C_FILES))

# ==============================
# Compilation des sous-modules
# ==============================
# echo "\n⚙️  Compilation du module $$(basename $$m)...";
modules:
	@echo "\n📦 Compilation des modules de controleur..."
	@for m in $(CONTROLEUR_MODULES); do \
		if echo "$(DISABLE)" | grep -qw "$$(basename $$m)"; then \
			echo "⏭️  Module $$(basename $$m) désactivé"; \
		else \
			$(MAKE) --no-print-directory -C $$m BUILD_DIR=$(CONTROLEUR_BUILD)/$$(basename $$m) INCLUDES="$(INCLUDES)" || \
				echo "⚠️  Erreur dans $$(basename $$m)"; \
		fi; \
	done

# ==============================
# Compilation des fichiers .c dans le dossier Controleur
# ==============================
$(CONTROLEUR_BUILD)/%.o: $(CONTROLEUR_DIR)/%.c
	@mkdir -p $(CONTROLEUR_BUILD)
# @echo "\n⚙️  Compilation des fichiers principaux"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@echo "✅ $$(basename $<) compilé"

dir_objs: $(CONTROLEUR_OBJS)

# ==============================
# Target principale pour le Makefile local
# ==============================
all: modules dir_objs
	@echo "Compilation terminée"

# ==============================
# Affichage pour debug
# ==============================
print-vars:
	@echo "----------------------------------------"
	@echo "🔧 VARIABLES DU MAKEFILE CONTROLEUR"
	@echo "CURDIR          = $(CURDIR)"
	@echo "CONTROLEUR_DIR     = $(CONTROLEUR_DIR)"
	@echo "CONTROLEUR_BUILD   = $(CONTROLEUR_BUILD)"
	@echo "INCLUDES        = $(INCLUDES)"
	@echo "CONTROLEUR_MODULES = $(CONTROLEUR_MODULES)"
	@echo "CONTROLEUR_C_FILES = $(CONTROLEUR_C_FILES)"
	@echo "CONTROLEUR_OBJS    = $(CONTROLEUR_OBJS)"
	@echo "----------------------------------------"
