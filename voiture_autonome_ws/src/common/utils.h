#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

// Si DEBUG_MODE est défini (dans le code ou via -DDEBUG_MODE), activer le debug
#ifdef DEBUG_MODE
    #define DEBUG_PRINT(fmt, ...) \
        do { fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while (0)
#else
    #define DEBUG_PRINT(fmt, ...) do {} while (0)
#endif

#endif