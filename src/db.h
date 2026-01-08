#ifndef DB_H
#define DB_H
#include <sqlite3.h>

int db_open(const char *path, sqlite3 **out);
int db_insert_measure(sqlite3 *db, int real_cm, int sensor_raw, const char *ts);
int db_insert_auto(sqlite3 *db, int sensor_raw, const char *ts);
int db_export_csv(sqlite3 *db, const char *csvfile);
int db_import_csv(sqlite3 *db, const char *csvfile);
int db_clear_table(sqlite3 *db);

#endif