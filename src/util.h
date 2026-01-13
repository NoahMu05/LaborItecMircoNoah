#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

// ASCII Histogramm (wert als double erlaubt)
void ascii_histogram_update(double value, int width, int height);

// Menü anzeigen
void print_menu(void);

// Zeitstempel erzeugen
void timestamp_now(char *buf, size_t len);

// Lese double-Wert von stdin mit Prompt
double read_double_stdin(const char *prompt);

// Non-blocking getchar (für q prüfen)
int nonblocking_getchar(void);

// Sleep ms
void sleep_ms(int ms);

#endif
