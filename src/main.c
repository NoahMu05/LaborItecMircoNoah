#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "serial.h"
#include "db.h"
#include "util.h"
#include "interp.h"
#include "filter.h"
#include "game.h"

int main(int argc, char *argv[]){
    const char *device = "/dev/ttyUSB0";
    int baud = 9600;
    const char *dbfile = "messung.db";

    if(argc >= 2) device = argv[1];
    if(argc >= 3) baud = atoi(argv[2]);

    int serial_fd = serial_open(device, baud);
    if(serial_fd < 0){
        fprintf(stderr, "Warnung: Serielles Gerät %s konnte nicht geöffnet werden. Einige Modi benötigen es.\n", device);
    } else {
        printf("Seriell geöffnet: %s @ %d\n", device, baud);
    }

    sqlite3 *db;
    if(db_open(dbfile, &db) != 0){
        fprintf(stderr, "DB Fehler\n");
        return 1;
    }

    while(1){
        print_menu();
        char line[32];
        if(!fgets(line, sizeof(line), stdin)) break;
        int opt = atoi(line);
        if(opt == 0) break;

        if(opt == 1){ // DA - manuell 20 Messungen
            printf("DA Modus: 20 Messungen. Gib reale Abstände in cm ein.\n");
            for(int i=0;i<20;i++){
                int real = read_int_stdin("Realer Abstand (cm): ");
                // read sensor value from serial
                int sensor = 0;
                char buf[128];
                printf("Warte auf Sensorwert...\n");
                while(1){
                    ssize_t n = serial_readline(serial_fd, buf, sizeof(buf));
                    if(n > 0){
                        sensor = atoi(buf);
                        break;
                    } else if(n == 0){
                        // timeout, prompt user to press Enter to retry or skip
                        printf("Kein Sensorwert empfangen. Drücke Enter um erneut zu versuchen.\n");
                        fgets(buf, sizeof(buf), stdin);
                    } else {
                        printf("Fehler beim Lesen der seriellen Schnittstelle\n");
                        break;
                    }
                }
                char ts[64];
                timestamp_now(ts, sizeof(ts));
                if(db_insert_measure(db, real, sensor, ts) == 0){
                    printf("Messung %d gespeichert: real=%d sensor=%d diff=%d\n", i+1, real, sensor, real - sensor);
                } else {
                    printf("Speichern fehlgeschlagen\n");
                }
            }
            // export CSV
            db_export_csv(db, "messung_1.csv");
            printf("messung_1.csv geschrieben\n");
        }
        else if(opt == 2){ // TEST - lookup + interpolation
            printf("TEST Modus: Importiere messung_1.csv in DB und baue Lookup\n");
            if(db_import_csv(db, "messung_1.csv") != 0) printf("Import fehlgeschlagen oder Datei nicht vorhanden\n");
            int *svals = NULL, *rvals = NULL, count = 0;
            if(build_lookup_from_csv("messung_1.csv", &svals, &rvals, &count) != 0){
                printf("Lookup Aufbau fehlgeschlagen\n");
            } else {
                printf("Lookup mit %d Punkten aufgebaut\n", count);
                // Beispiel: lese serielle Werte und berechne interpolierten Wert
                printf("Drücke q um zurück zum Menü. Lese Sensorwerte und zeige interpolierten Abstand.\n");
                char buf[128];
                while(1){
                    ssize_t n = serial_readline(serial_fd, buf, sizeof(buf));
                    if(n > 0){
                        int sensor = atoi(buf);
                        double real = interpolate_from_lookup(sensor, svals, rvals, count);
                        char ts[64]; timestamp_now(ts, sizeof(ts));
                        // speichere korrigierten Wert als real_cm
                        db_insert_measure(db, (int) (real + 0.5), sensor, ts);
                        printf("Sensor=%d -> interpoliert=%.2f cm\n", sensor, real);
                    } else if(n == 0){
                        // no data, check for user quit
                        int c = nonblocking_getchar();
                        if(c=='q' || c=='Q') break;
                        sleep_ms(200);
                    } else {
                        printf("Serial read error\n");
                        break;
                    }
                }
                free_lookup(svals, rvals);
                db_export_csv(db, "messung_2.csv");
                printf("messung_2.csv geschrieben\n");
            }
        }
        else if(opt == 3){ // AUTO
            printf("AUTO Modus: Messe automatisch für ca. 10 Sekunden\n");
            int duration_ms = 10000;
            int interval_ms = 100; // try to read frequently
            int elapsed = 0;
            char buf[128];
            int prev_ts = 0;
            int count = 0;
            double sum_dt = 0;
            int min_dt = 1000000, max_dt = 0;
            while(elapsed < duration_ms){
                ssize_t n = serial_readline(serial_fd, buf, sizeof(buf));
                if(n > 0){
                    int sensor = atoi(buf);
                    char ts[64]; timestamp_now(ts, sizeof(ts));
                    db_insert_auto(db, sensor, ts);
                    // histogram update
                    system("clear");
                    printf("AUTO Messung: sensor=%d\n", sensor);
                    ascii_histogram_update(sensor, 40, 10);
                    // timing stats
                    int now_ms = elapsed;
                    if(prev_ts != 0){
                        int dt = now_ms - prev_ts;
                        if(dt < min_dt) min_dt = dt;
                        if(dt > max_dt) max_dt = dt;
                        sum_dt += dt;
                    }
                    prev_ts = now_ms;
                    count++;
                }
                sleep_ms(interval_ms);
                elapsed += interval_ms;
            }
            if(count > 1){
                double avg = sum_dt / (count - 1);
                printf("Abtastzeit min=%d ms max=%d ms avg=%.2f ms\n", min_dt, max_dt, avg);
            }
            db_export_csv(db, "messung_3.csv");
            printf("messung_3.csv geschrieben\n");
        }
        else if(opt == 4){ // FILTER
            printf("FILTER Modus: Lese messung_3.csv, wende Mittelwertfilter (3) an und schreibe messung_3_filtered.csv\n");
            // load sensor_raw from messung_3.csv
            FILE *f = fopen("messung_3.csv", "r");
            if(!f){ printf("messung_3.csv nicht gefunden. Führe AUTO zuerst aus.\n"); continue; }
            char line[512];
            if(!fgets(line, sizeof(line), f)){ fclose(f); continue; }
            int cap = 256, n = 0;
            int *vals = malloc(sizeof(int)*cap);
            char ts[64];
            while(fgets(line, sizeof(line), f)){
                char *tok = strtok(line, ","); // id
                if(!tok) continue;
                tok = strtok(NULL, ","); // real_cm
                int real = 0;
                if(tok && strlen(tok)>0) real = atoi(tok);
                tok = strtok(NULL, ","); // sensor_raw
                int sensor = 0;
                if(tok) sensor = atoi(tok);
                int v = (real>0)? real : sensor;
                if(n >= cap){ cap *= 2; vals = realloc(vals, sizeof(int)*cap); }
                vals[n++] = v;
            }
            fclose(f);
            double *out = malloc(sizeof(double)*n);
            moving_average_3(vals, n, out);
            // write CSV with raw, filtered, diff
            FILE *fo = fopen("messung_3_filtered.csv", "w");
            fprintf(fo, "index,raw,filtered,diff\n");
            for(int i=0;i<n;i++){
                fprintf(fo, "%d,%d,%.3f,%.3f\n", i+1, vals[i], out[i], vals[i]-out[i]);
            }
            fclose(fo);
            free(vals); free(out);
            printf("messung_3_filtered.csv geschrieben\n");
        }
        else if(opt == 5){ // FUN
            printf("FUN Modus: Starte Spiel mit Profil aus messung_3.csv\n");
            play_game_from_profile("messung_3.csv");
        }
        else if(opt == 6){ // EXPORT
            printf("Exportiere DB als messung_export.csv\n");
            db_export_csv(db, "messung_export.csv");
            printf("messung_export.csv geschrieben\n");
        }
        else if(opt == 7){ // CLEAR
            db_clear_table(db);
            printf("Messdaten gelöscht\n");
        }
        else {
            printf("Unbekannte Option\n");
        }
    }

    if(serial_fd >= 0) serial_close(serial_fd);
    sqlite3_close(db);
    printf("Beende Programm\n");
    return 0;
}