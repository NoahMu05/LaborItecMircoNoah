#ifndef AUTOMODE_H
#define AUTOMODE_H

#include <stddef.h>
#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif


int automode_run(int serial_fd,
                 int MAX_MEASUREMENTS,
                 char *chunk,
                 char *line_buffer,
                 size_t *line_len);

#ifdef __cplusplus
}
#endif

#endif 