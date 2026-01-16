/* da.c */
#include "da.h"
#include "itec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <math.h>
#include <sqlite3.h>

int da_run(int serial_fd,
           int MAX_MEASUREMENTS,
           char *chunk,
           char *line_buffer,
           size_t *line_len)
{
    sqlite3 *db = NULL;
    if (open_database(&db, "Messung1.db") != 0) {
        return 1;
    }

    execute_sql(db, "DROP TABLE IF EXISTS Messung1;");
    execute_sql(db,
        "CREATE TABLE IF NOT EXISTS Messung1 ("
        "Messung_Nr INTEGER PRIMARY KEY, "
        "Abstand_Real DOUBLE, "
        "Abstand_Sensor DOUBLE, "
        "Abweichung DOUBLE);");

    double gemessener_abstand[20];
    char eingelesen[100];

    printf("Geben Sie 20 Messwerte in cm ein:\n\n");

    int m = 0;
    while (m < 20) {
        printf("Wert %d: ", m + 1);
        if (fgets(eingelesen, sizeof(eingelesen), stdin) == NULL) {
            printf("Fehler beim Lesen!\n");
            sqlite3_close(db);
            return 1;
        }

        gemessener_abstand[m] = atof(eingelesen);

        float sensorwert = -1.0f;

        tcflush(serial_fd, TCIFLUSH);

        if (line_len) *line_len = 0;
        if (line_buffer) line_buffer[0] = '\0';

        usleep(200000);

        while (sensorwert < 0) {
            sensorwert = read_sensor_value(serial_fd, chunk, line_buffer, line_len);
        }

        printf("Sensor Value: %.3f cm\n", sensorwert);

        double abweichung = gemessener_abstand[m] - sensorwert;

        char temp[256];
        snprintf(temp, sizeof(temp),
            "INSERT INTO Messung1 (Abstand_Real, Abstand_Sensor, Abweichung) VALUES (%f,%f,%f);",
            gemessener_abstand[m], sensorwert, abweichung);
        execute_sql(db, temp);

        m++;
    }

    execute_sql_csv("Messung1.csv", db, "SELECT * FROM Messung1;");

    if (db) {
        sqlite3_close(db);
        db = NULL;
    }

    return 0;
}