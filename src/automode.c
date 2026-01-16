#include "automode.h"
#include "itec.h"      
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

static void strip_crlf(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) s[--len] = '\0';
}

int automode_run(int serial_fd,
                 int MAX_MEASUREMENTS,
                 char *chunk,
                 char *line_buffer,
                 size_t *line_len)
{
    bool debug = false;
    double min_dt = 999999.0;
    double max_dt = 0.0;
    double sum_dt = 0.0;
    long long count_dt = 0;

    struct timeval last_time, current_time;
    gettimeofday(&last_time, NULL);

    sqlite3 *db = NULL;

    if (open_database(&db, "LookUpTabelle.db") != 0)
        return 1;

    if (!create_lookup_table(db, "Messung1.csv")) {
        sqlite3_close(db);
        return 1;
    }

    execute_sql(db, "DROP TABLE IF EXISTS Messung3;");

    execute_sql(db,
        "CREATE TABLE IF NOT EXISTS Messung3 ("
        "Messung_Nr INTEGER PRIMARY KEY, "
        "Zeitstempel TEXT, "
        "Abstand_Sensor DOUBLE, "
        "interpolierter_Wert DOUBLE, "
        "Abweichung DOUBLE, "
        "ProfilHoehe DOUBLE);");

    printf("Auto Messung gestartet. Drücke q zum Abbrechen...\n");

    set_input_mode();
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    int start = 0;
    while (1)
    {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0)
        {
            if (c == 'q')
            {
                printf("\nBeende AUTO-Modus...\n");
                break;
            }
        }

        float sensorwert = -1.0f;
        while (sensorwert < 0)
            sensorwert = read_sensor_value(serial_fd, chunk, line_buffer, line_len);

        printf(" %.3f cm\n", sensorwert);

        gettimeofday(&current_time, NULL);

        double dt =
            (current_time.tv_sec - last_time.tv_sec) +
            (current_time.tv_usec - last_time.tv_usec) / 1e6;

        last_time = current_time;

        if (dt < min_dt) min_dt = dt;
        if (dt > max_dt) max_dt = dt;

        sum_dt += dt;
        count_dt++;

        double interpolated_value = Interpolate_measurement(db, sensorwert, debug);
        if (isnan(interpolated_value)) {
            printf("Interpolation fehlgeschlagen.\n");
            continue;
        }

        double abweichung = interpolated_value - sensorwert;

        double Hoehe = 13;
        if (interpolated_value > Hoehe) {
            interpolated_value = Hoehe;
        }
        double profilhoehe = Hoehe - interpolated_value;

        char temp[256];
        snprintf(temp, sizeof(temp),
            "INSERT INTO Messung3 (Zeitstempel, Abstand_Sensor, interpolierter_Wert, Abweichung, ProfilHoehe) "
            "VALUES (DATETIME('now'),%f,%f,%f,%f);",
            sensorwert, interpolated_value, abweichung, profilhoehe);

        execute_sql(db, temp);

        execute_sql_csv("Messung3.csv", db, "SELECT * FROM Messung3;");

        int *histogram = malloc(sizeof(int) * (size_t)MAX_MEASUREMENTS);
        if (!histogram) {
            fprintf(stderr, "Speicherfehler für Histogramm\n");
            break;
        }
        int length = 0;

        FILE *fp = fopen("Messung3.csv", "r");
        if (!fp) {
            printf("CSV konnte nicht geöffnet werden!\n");
            free(histogram);
            break;
        }

        char line[256];
        if (fgets(line, sizeof(line), fp)) {
           
        }

        while (fgets(line, sizeof(line), fp))
        {
            int nr;
            char timestamp[64];
            float sensor, interpoliert, abw_csv, profilhoehe_csv;

            if (sscanf(line, "%d,%63[^,],%f,%f,%f,%f",
                    &nr, timestamp, &sensor, &interpoliert, &abw_csv, &profilhoehe_csv) == 6)
            {
                if (length >= MAX_MEASUREMENTS)
                    break;

                if (profilhoehe_csv < 0)
                    profilhoehe_csv = 0;
                int value = (int)round(profilhoehe_csv);
                if (value < 0) value = 0;

                histogram[length++] = value;
            }
        }

        fclose(fp);

        struct winsize w;
        get_terminal_dim(&w);

        int end = 0;

        if (length <= w.ws_col)
        {
            end = length;
            system("clear");
            print_hist(length, histogram, start, end);
        }
        else
        {
            do
            {
                end = start + w.ws_col;
                system("clear");
                print_hist(length, histogram, start, end);
                start++;
                usleep(100 * 500);
            } while (end <= length);
        }

        free(histogram);
    }

    reset_input_mode();
    tcflush(STDIN_FILENO, TCIFLUSH);

    int flags = fcntl(STDIN_FILENO, F_GETFL);
    flags &= ~O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flags);

    printf("\n\n--- Abtastzeit-Statistik ---\n");
    printf("Minimale Abtastzeit: %.6f s\n", min_dt);
    printf("Maximale Abtastzeit: %.6f s\n", max_dt);

    if (count_dt > 0)
        printf("Durchschnittliche Abtastzeit: %.6f s\n", sum_dt / count_dt);
    else
        printf("Durchschnittliche Abtastzeit: keine Messungen\n");

    printf("-----------------------------\n");

    if (db) {
        sqlite3_close(db);
        db = NULL;
    }

    return 0;
}