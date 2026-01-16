#ifndef AUTOMODE_H
#define AUTOMODE_H

#include <stddef.h>
#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Automode starten.
 * Parameter-Typen so gewählt, dass sie mit den Deklarationen in itec.h kompatibel sind:
 *  - serial_fd: int (Dateideskriptor)
 *  - MAX_MEASUREMENTS: int
 *  - chunk: char* (Puffer/Chunk-Parameter, falls dein Aufruf so ist)
 *  - line_buffer: char* (Puffer für serielle Zeilen)
 *  - line_len: size_t* (Länge des gelesenen Puffers, passt zu deiner Verwendung)
 */
int automode_run(int serial_fd,
                 int MAX_MEASUREMENTS,
                 char *chunk,
                 char *line_buffer,
                 size_t *line_len);

#ifdef __cplusplus
}
#endif

#endif /* AUTOMODE_H */