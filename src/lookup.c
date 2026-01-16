#include "lookup.h"
#include "itec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <math.h>
#include <sqlite3.h>

int lookup_run(int serial_fd,
               int MAX_MEASUREMENTS,
               char *chunk,
               char *line_buffer,
               size_t *line_len)
{
    bool debug = true;

    sqlite3 *db = NULL;
    if (open_database(&db, "LookUpTabelle.db") != 0) {
        return 1;
    }

    if (!create_lookup_table(db, "Messung1.csv")) {
        sqlite3_close(db);
        return 1;
    }

    execute_sql(db, "DROP TABLE IF EXISTS Messung2;");

    execute_sql(db,
        "CREATE TABLE IF NOT EXISTS Messung2 ("
        "Messung_Nr INTEGER PRIMARY KEY, "
        "Abstand_Sensor DOUBLE, "
        "interpolierter_Wert DOUBLE, "
        "Abweichung DOUBLE);");

    printf("Drücke Enter zum Start der Messung und q um die Messung abzubrechen...\n");

    int m = 1;
    while (1)
    {
        char enter[16];
        printf("Messung %d: ", m);
        if (!fgets(enter, sizeof(enter), stdin)) {
            clearerr(stdin);
            continue;
        }

        if (strcmp(enter, "q\n") == 0)
            break;

        if (strcmp(enter, "\n") != 0) {
            printf("Ungültige Eingabe.\n");
            continue;
        }

        float sensorwert = -1.0f;

        tcflush(serial_fd, TCIFLUSH);

        if (line_len) *line_len = 0;
        if (line_buffer) line_buffer[0] = '\0';

        usleep(200000);
        while (sensorwert < 0)
            sensorwert = read_sensor_value(serial_fd, chunk, line_buffer, line_len);

        printf(" %.3f cm\n", sensorwert);

        double interpolated_value = Interpolate_measurement(db, sensorwert, debug);
        if (isnan(interpolated_value)) {
            printf("Interpolation fehlgeschlagen.\n");
            continue;
        }

        double abweichung = interpolated_value - sensorwert;

        char temp[512];
        snprintf(temp, sizeof(temp),
            "INSERT INTO Messung2 (Abstand_Sensor, interpolierter_Wert, Abweichung) "
            "VALUES (%f,%f,%f);",
            sensorwert, interpolated_value, abweichung);

        execute_sql(db, temp);

        m++;
    }

    execute_sql_csv("Messung2.csv", db, "SELECT * FROM Messung2;");

    if (db) {
        sqlite3_close(db);
        db = NULL;
    }

    return 0;
}