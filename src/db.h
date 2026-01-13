#ifndef DB_H
#define DB_H

#include <sqlite3.h>

// Öffnet die Datenbank und erstellt Tabelle (REAL statt INTEGER)
int db_open(const char *path, sqlite3 **out);

// Speichert Messwerte (real_cm, sensor_raw, diff als double)
int db_insert_measure(sqlite3 *db, double real_cm, double sensor_raw, const char *ts);

// Speichert automatische Messwerte (nur sensor_raw als double)
int db_insert_auto(sqlite3 *db, double sensor_raw, const char *ts);

// Exportiert komplette Tabelle als CSV
int db_export_csv(sqlite3 *db, const char *csvfile);

// Importiert CSV (real_cm und sensor_raw als double)
int db_import_csv(sqlite3 *db, const char *csvfile);

// Löscht alle Messdaten
int db_clear_table(sqlite3 *db);

#endif
