#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int db_open(const char *path, sqlite3 **out){
    if(sqlite3_open(path, out) != SQLITE_OK){
        fprintf(stderr, "Cannot open DB: %s\n", sqlite3_errmsg(*out));
        return -1;
    }
    const char *sql =
        "CREATE TABLE IF NOT EXISTS measurements("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "real_cm INTEGER,"
        "sensor_raw INTEGER NOT NULL,"
        "diff INTEGER,"
        "ts TEXT NOT NULL"
        ");";
    if(sqlite3_exec(*out, sql, 0,0,0) != SQLITE_OK){
        fprintf(stderr, "Create table failed: %s\n", sqlite3_errmsg(*out));
        return -1;
    }
    return 0;
}

int db_insert_measure(sqlite3 *db, int real_cm, int sensor_raw, const char *ts){
    const char *sql = "INSERT INTO measurements(real_cm,sensor_raw,diff,ts) VALUES(?,?,?,?);";
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt,1,real_cm);
    sqlite3_bind_int(stmt,2,sensor_raw);
    sqlite3_bind_int(stmt,3, real_cm - sensor_raw);
    sqlite3_bind_text(stmt,4, ts, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_insert_auto(sqlite3 *db, int sensor_raw, const char *ts){
    const char *sql = "INSERT INTO measurements(real_cm,sensor_raw,diff,ts) VALUES(NULL,?,NULL,?);";
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt,1,sensor_raw);
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
        const unsigned char *real = sqlite3_column_text(stmt,1);
        int sensor = sqlite3_column_int(stmt,2);
        const unsigned char *diff = sqlite3_column_text(stmt,3);
        const unsigned char *ts = sqlite3_column_text(stmt,4);
        fprintf(f, "%d,%s,%d,%s,%s\n",
            id,
            real ? (const char*)real : "",
            sensor,
            diff ? (const char*)diff : "",
            ts ? (const char*)ts : "");
    }
    sqlite3_finalize(stmt);
    fclose(f);
    return 0;
}

int db_import_csv(sqlite3 *db, const char *csvfile){
    FILE *f = fopen(csvfile, "r");
    if(!f) return -1;
    char line[512];
    // skip header
    if(!fgets(line, sizeof(line), f)){ fclose(f); return -1; }
    while(fgets(line, sizeof(line), f)){
        int id, real_cm, sensor_raw;
        char ts[64];
        // Accept lines with at least real_cm and sensor_raw or sensor_raw only
        // Format expected: id,real_cm,sensor_raw,diff,ts
        char *p = line;
        // parse by tokenizing
        char *tok;
        tok = strtok(p, ","); // id
        if(!tok) continue;
        tok = strtok(NULL, ","); // real_cm
        if(!tok) continue;
        real_cm = atoi(tok);
        tok = strtok(NULL, ","); // sensor_raw
        if(!tok) continue;
        sensor_raw = atoi(tok);
        tok = strtok(NULL, ","); // diff
        tok = strtok(NULL, ",\n"); // ts
        if(tok) strncpy(ts, tok, sizeof(ts)-1);
        else ts[0]=0;
        // insert
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