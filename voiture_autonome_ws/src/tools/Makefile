# ==============================
# Makefile principal des TOOLS
# ==============================

# Compilation
CC := gcc
CFLAGS := -Wall -O2 -pthread

# Répertoire de build fourni par le Makefile parent (chemin absolu) : BUILD_DIR

# Liste des sous-dossiers
TOOL_DIRS := $(wildcard $(CURDIR)/*/ )

all: 
#	@echo "🧰 Compilation des outils..."
	@for dir in $(TOOL_DIRS); do \
		if [ -f "$$dir/Makefile" ]; then \
			$(MAKE) -C $$dir BUILD_DIR=$(BUILD_DIR)/$$(basename $$dir) INCLUDES_TOOLS=$(INCLUDES_TOOLS) || echo "⚠️ Erreur ignorée dans $$dir"; \
		else \
			echo "[tools/$$(basename $$dir)]  ⏭️  Aucun Makefile dans tools/$$(basename $$dir)"; \
		fi; \
	done
	@echo "[tools] Compilation des tools terminée"