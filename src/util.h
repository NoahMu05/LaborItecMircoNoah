#ifndef UTIL_H
#define UTIL_H
#include <sqlite3.h>
#include <stddef.h>

void ascii_histogram_update(int value, int width, int height);
void print_menu();
void timestamp_now(char *buf, size_t len);
int read_int_stdin(const char *prompt);
int nonblocking_getchar();
void sleep_ms(int ms);
#endif