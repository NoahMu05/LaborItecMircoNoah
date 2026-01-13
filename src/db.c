//Datenbank: erstellt Tabelle und fügt Messwerte ein;
//exportiert Daten als CSV und importiert CSVs für Lookup/ Interpolation
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

int db_open(const char *path, sqlite3 **out){
    if(sqlite3_open(path, out) != SQLITE_OK){
        fprintf(stderr, "Cannot open DB: %s\n", sqlite3_errmsg(*out));
        return -1;
    }
    const char *sql =
        "CREATE TABLE IF NOT EXISTS measurements("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "real_cm REAL,"
        "sensor_raw REAL NOT NULL,"
        "diff REAL,"
        "ts TEXT NOT NULL"
        ");";
    if(sqlite3_exec(*out, sql, 0,0,0) != SQLITE_OK){
        fprintf(stderr, "Create table failed: %s\n", sqlite3_errmsg(*out));
        return -1;
    }
    return 0;
}

int db_insert_measure(sqlite3 *db, double real_cm, double sensor_raw, const char *ts){
    const char *sql = "INSERT INTO measurements(real_cm,sensor_raw,diff,ts) VALUES(?,?,?,?);";
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    sqlite3_bind_double(stmt,1,real_cm);
    sqlite3_bind_double(stmt,2,sensor_raw);
    sqlite3_bind_double(stmt,3, real_cm - sensor_raw);
    sqlite3_bind_text(stmt,4, ts, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_insert_auto(sqlite3 *db, double sensor_raw, const char *ts){
    const char *sql = "INSERT INTO measurements(real_cm,sensor_raw,diff,ts) VALUES(NULL,?,NULL,?);";
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    sqlite3_bind_double(stmt,1,sensor_raw);
    sqlite3_bind_text(stmt,2, ts, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_export_csv(sqlite3 *db, const char *csvfile){
    FILE *f = fopen(csvfile, "w");
    if(!f) return -1;
    fprintf(f, "id,real_cm,sensor_raw,diff,ts\n");
    const char *sql = "SELECT id,real_cm,sensor_raw,diff,ts FROM measurements ORDER BY id;";
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK){ fclose(f); return -1; }
    while(sqlite3_step(stmt) == SQLITE_ROW){
        int id = sqlite3_column_int(stmt,0);
        int type_real = sqlite3_column_type(stmt,1);
        double reald = sqlite3_column_double(stmt,1);
        double sensor = sqlite3_column_double(stmt,2);
        int type_diff = sqlite3_column_type(stmt,3);
        double diffd = sqlite3_column_double(stmt,3);
        const unsigned char *ts = sqlite3_column_text(stmt,4);
        if(type_real == SQLITE_NULL){
            fprintf(f, "%d,,%lf,", id, sensor);
        } else {
            fprintf(f, "%d,%.6f,%lf,%.6f,", id, reald, sensor, diffd);
        }
        fprintf(f, "%s\n", ts ? (const char*)ts : "");
    }
    sqlite3_finalize(stmt);
    fclose(f);
    return 0;
}

int db_import_csv(sqlite3 *db, const char *csvfile){
    FILE *f = fopen(csvfile, "r");
    if(!f) return -1;
    char line[512];

    if(!fgets(line, sizeof(line), f)){ fclose(f); return -1; }
    while(fgets(line, sizeof(line), f)){
        int id = 0;
        double real_cm = 0.0, sensor_raw = 0.0;
        char ts[128] = {0};

        char *p = line;
        char *tok;

        tok = strtok(p, ","); // id
        if(!tok) continue;
        id = atoi(tok);
        tok = strtok(NULL, ","); // real_cm
        if(tok && strlen(tok)>0) real_cm = atof(tok);
        tok = strtok(NULL, ","); // sensor_raw
        if(tok && strlen(tok)>0) sensor_raw = atof(tok);
        // skip diff
        tok = strtok(NULL, ",");
        // ts (rest)
        tok = strtok(NULL, ",\n");
        if(tok) strncpy(ts, tok, sizeof(ts)-1);
        else ts[0]=0;

        db_insert_measure(db, real_cm, sensor_raw, ts[0]?ts:"now");
    }
    fclose(f);
    return 0;
}

int db_clear_table(sqlite3 *db){
    const char *sql = "DELETE FROM measurements;";
    if(sqlite3_exec(db, sql, 0,0,0) != SQLITE_OK) return -1;
    return 0;
}
