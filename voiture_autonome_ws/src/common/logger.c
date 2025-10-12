#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#if LOG_TO_FILE
static FILE* log_file = NULL;
static FILE* err_file = NULL;

static void init_log_files() {
    if (log_file && err_file) return;
    printf("Création des fichiers de log\n");

    // Crée le dossier log/ si nécessaire
    int ret = system("mkdir -p log");
    if (ret != 0) {
        fprintf(stderr, "Erreur lors de la création du dossier log\n");
    }

    char path[512];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // Nom du fichier avec date + heure + seconde
    snprintf(path, sizeof(path), "log/%04d-%02d-%02d_%02d-%02d-%02d.txt",
             tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    log_file = fopen(path, "a");
    err_file = fopen(path, "a");  // même fichier pour simplifier

    if (!log_file || !err_file) {
        fprintf(stderr, "Impossible d'ouvrir le fichier de log\n");
        log_file = err_file = NULL;
    }
}

#endif

void _log(char level, const char* prefix, const char* fmt, ...) {
    va_list args;
    FILE* out = (level == 'E') ? stderr : stdout;

    const char* color =
        (level == 'E') ? COLOR_ERR :
        (level == 'W') ? COLOR_WARN :
        (level == 'D') ? COLOR_DBG :
                         COLOR_INFO;

    fprintf(out, "%s[%c:%s]%s ", color, level, prefix, COLOR_RESET);

    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
    fflush(out);

#if LOG_TO_FILE
    init_log_files();
    if (log_file && err_file) {
        FILE* f = (level == 'E') ? err_file : log_file;
        va_start(args, fmt);
        fprintf(f, "[%c:%s] ", level, prefix);
        vfprintf(f, fmt, args);
        va_end(args);
        fprintf(f, "\n");
        fflush(f);
    }
#endif
}
