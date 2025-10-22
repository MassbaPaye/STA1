# ==============================
# Makefile pour src/voiture
# ==============================

# Variables fournies par le Makefile global
#		CFLAGS="$(CFLAGS)" \
		VOITURE_BUILD=$(VOITURE_BUILD) \
		COMMON_INCLUDES="$(INCLUDES_COMMON)" \
		TOOLS_INCLUDES="$(INCLUDES_TOOLS)" \
		DISABLE="$(DISABLE)" \


# Compilateur
CC := gcc

# Variables pour le build
VOITURE_DIR := $(CURDIR)


# Liste des sous-dossiers (= modules) avec chemin complet
VOITURE_MODULES := \
	$(VOITURE_DIR)/CommunicationSerie/ \
	$(VOITURE_DIR)/CommunicationTCP/ \
	$(VOITURE_DIR)/CommunicationUDP/ \
	$(VOITURE_DIR)/GestionComportement/ \
	$(VOITURE_DIR)/Localisation/ \
	$(VOITURE_DIR)/ModuleTemplate/ \
	$(VOITURE_DIR)/Simulation/ \
	$(VOITURE_DIR)/SuiviTrajectoire/

INCLUDES = $(COMMON_INCLUDES) $(TOOLS_INCLUDES) -I$(VOITURE_DIR) $(addprefix -I, $(VOITURE_MODULES))

# Fichiers .c dans VOITURE_DIR (hors sous-dossiers)
VOITURE_C_FILES := $(wildcard $(VOITURE_DIR)/*.c)
VOITURE_OBJS := $(patsubst $(VOITURE_DIR)/%.c,$(VOITURE_BUILD)/%.o,$(VOITURE_C_FILES))

# ==============================
# Compilation des sous-modules
# ==============================
# echo "\n⚙️  Compilation du module $$(basename $$m)...";
modules:
	@echo "📦 Compilation des modules de voiture..."
	@echo "Modules détectés: $(VOITURE_MODULES)"
	@for m in $(VOITURE_MODULES); do \
		if echo "$(DISABLE)" | grep -qw "$$(basename $$m)"; then \
			echo "⏭️  Module $$(basename $$m) désactivé"; \
		else \
			echo "⚙️  Compilation du module $$(basename $$m) → BUILD_DIR=$(VOITURE_BUILD)/$$(basename $$m)"; \
			$(MAKE) --no-print-directory -C $$m CFLAGS="$(CFLAGS)" BUILD_DIR=$(VOITURE_BUILD)/$$(basename $$m) INCLUDES="$(INCLUDES)" || \
				echo "⚠️  Erreur dans $$(basename $$m)"; \
		fi; \
	done

# ==============================
# Compilation des fichiers .c dans le dossier voiture
# ==============================
$(VOITURE_BUILD)/%.o: $(VOITURE_DIR)/%.c
	@mkdir -p $(VOITURE_BUILD)
	@echo "\n⚙️  Compilation des fichiers principaux"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@echo "✅ $$(basename $<) compilé"

dir_objs: $(VOITURE_OBJS)

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
	@echo "🔧 VARIABLES DU MAKEFILE VOITURE"
	@echo "CURDIR          = $(CURDIR)"
	@echo "VOITURE_DIR     = $(VOITURE_DIR)"
	@echo "VOITURE_BUILD   = $(VOITURE_BUILD)"
	@echo "INCLUDES        = $(INCLUDES)"
	@echo "VOITURE_MODULES = $(VOITURE_MODULES)"
	@echo "VOITURE_C_FILES = $(VOITURE_C_FILES)"
	@echo "VOITURE_OBJS    = $(VOITURE_OBJS)"
	@echo "----------------------------------------"
