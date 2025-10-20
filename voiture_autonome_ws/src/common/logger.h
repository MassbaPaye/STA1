#pragma once
#include <stdio.h>
#include <stdarg.h>

// === Couleurs ANSI ===
#define COLOR_RESET   "\033[0m"
#define COLOR_INFO    "\033[32m"  // vert
#define COLOR_WARN    "\033[33m"  // jaune
#define COLOR_ERR     "\033[31m"  // rouge
#define COLOR_DBG     "\033[36m"  // cyan

// === Niveau de log ===
// 1 = normal (INFO, WARN, ERR)
// 2 = debug
#ifndef LOG_LEVEL
#define LOG_LEVEL 1
#endif

// === Activation de l'écriture dans fichiers ===
#ifndef LOG_TO_FILE
#define LOG_TO_FILE 0
#endif

// === Fonction interne ===
void _log(char level, const char* prefix, const char* fmt, ...);

// === Macros publiques ===
#define INFO(prefix, fmt, ...)  _log('I', prefix, fmt, ##__VA_ARGS__)
#define WARN(prefix, fmt, ...)  _log('W', prefix, fmt, ##__VA_ARGS__)
#define ERR(prefix, fmt, ...)   _log('E', prefix, fmt, ##__VA_ARGS__)

#if LOG_LEVEL >= 2
#define DEBUG
#define DBG(prefix, fmt, ...)   _log('D', prefix, fmt, ##__VA_ARGS__)
#else
#define DBG(prefix, fmt, ...)   ((void)0)
#endif
