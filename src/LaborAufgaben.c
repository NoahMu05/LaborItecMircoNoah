#include "itec.h"
#include "filter.h"
#include "automode.h"

#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/time.h>


//Um die Simulation der Sensorwerte anzuschalten muss "SIM 0" gesetzt werden.
//#define SIM 1

#define MAX_MEASUREMENTS 1000

double min_dt = 999999.0;
double max_dt = 0.0;
double sum_dt = 0.0;
long count_dt = 0;

void set_input_mode(void)
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void reset_input_mode(void)
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}



int main(int argc, char *argv[]) 
{   
    while (1) 
    {
        #if SIM
            char* tty_path = "/dev/ttyUSB0";
            int serial_fd = connect_to_sensor(tty_path);
        #else
            int serial_fd = 0;    
        #endif

        setvbuf(stdout, NULL, _IONBF, 0);

        //Buffer zum Lesen und Verarbeiten von Daten pro Sensor
        char chunk[READ_CHUNK];
        char line_buffer[2*READ_CHUNK];
        size_t line_len = 0;

        //Datenbank öffnen
        sqlite3 *db;

        //Modus auswählen
        char modus[100];
        printf("\nWelchen Modus willst du nutzen:\nSTOP:1 \nDA:2 \nTEST:3 \nAUTO:4 \nFILTER:5 \nFUN:6\n " );

        fgets(modus, sizeof(modus), stdin); 
        modus[strcspn(modus, "\n")] = '\0';

        if (strcmp(modus, "1") == 0) //STOP
        {
            printf("Programm wird beendet.\n");
            break; 
        }
        else if (strcmp(modus, "2") == 0) //Aufgabe 1: DA
        {

            if (open_database(&db, "Messung1.db") != 0) 
            {
                return 1;
            }

            // Alte Tabelle löschen und neu erstellen
            execute_sql(db, "DROP TABLE IF EXISTS Messung1;");

            //Tabelle erstellen
            execute_sql(db,
                "CREATE TABLE IF NOT EXISTS Messung1 (Messung_Nr INTEGER PRIMARY KEY, Abstand_Real DOUBLE, Abstand_Sensor DOUBLE, Abweichung DOUBLE);");

            double gemessener_abstand[20];
            char eingelesen[100];

            printf("Geben Sie 20 Messwerte in cm ein:\n\n");


            int m = 0;
            while (m < 20) 
            {
                printf("Wert %d: ", m + 1);

                if (fgets(eingelesen, sizeof(eingelesen), stdin) == NULL) {
                    printf("Fehler beim Lesen!\n");
                    return 1;
                }

                gemessener_abstand[m] = atof(eingelesen);  
               
                float sensorwert = -1.0f; 
                
                tcflush(serial_fd, TCIFLUSH);

                line_len = 0;
                line_buffer[0] = '\0';

                usleep(200000);

                while (sensorwert < 0) 
                { 
                    sensorwert = read_sensor_value(serial_fd, chunk, line_buffer, &line_len); 
                }

                printf("Sensor Value: %.3f cm\n", sensorwert);
            
                double abweichung = gemessener_abstand[m] - sensorwert;

                char temp[256];

                sprintf(temp, "INSERT INTO Messung1 (Abstand_Real, Abstand_Sensor, Abweichung) VALUES (%f,%f,%f);", gemessener_abstand[m], sensorwert, abweichung);
                execute_sql(db, temp);
                
                m++;
            }

            execute_sql_csv("Messung1.csv",db,
                "SELECT * FROM Messung1;");

        } 
        else if (strcmp(modus, "3") == 0) //Aufgabe 2:LookUpTabelle erstellen
        {
            bool debug = true;

            if (open_database(&db, "LookUpTabelle.db") != 0) 
            {
                return 1;
            }

            if (!create_lookup_table(db, "Messung1.csv")) 
            return 1;

            execute_sql(db, "DROP TABLE IF EXISTS Messung2;");

            execute_sql(db,
                "CREATE TABLE IF NOT EXISTS Messung2 (Messung_Nr INTEGER PRIMARY KEY, Abstand_Sensor DOUBLE, interpolierter_Wert DOUBLE, Abweichung DOUBLE);");

            
            printf("Drücke Enter zum Start der Messung und q um die Messung abzubrechen...\n"); 

            int m = 1;
            while (1)
            {
                char enter[10];
                printf("Messung %d: ", m);
                fgets(enter, sizeof(enter), stdin);

                if (strcmp(enter, "q\n") == 0)
                    break;

                if (strcmp(enter, "\n") != 0) 
                {
                    printf("Ungültige Eingabe.\n");
                    continue;
                }

                float sensorwert = -1.0f;
                
                tcflush(serial_fd, TCIFLUSH);

                line_len = 0;
                line_buffer[0] = '\0';

                usleep(200000); 
                while (sensorwert < 0)
                    sensorwert = read_sensor_value(serial_fd, chunk, line_buffer, &line_len);

                printf(" %.3f cm\n", sensorwert);

                double interpolated_value = Interpolate_measurement(db, sensorwert, debug);
                if (isnan(interpolated_value)) {
                    printf("Interpolation fehlgeschlagen.\n");
                    continue;
                }

                double abweichung = interpolated_value - sensorwert;

                // In DB 
                char temp[256];
                sprintf(temp,
                    "INSERT INTO Messung2 (Abstand_Sensor, interpolierter_Wert, Abweichung) "
                    "VALUES (%f,%f,%f);",
                    sensorwert, interpolated_value, abweichung);

                execute_sql(db, temp);

                m++;
            }

            //Tabelle in eine .csv Datei
            execute_sql_csv("Messung2.csv",db,
                "SELECT * FROM Messung2;");
    
        } 


        else if (strcmp(modus, "4") == 0) // Aufgabe 3:Auto Messung
        {
            automode_run(serial_fd, MAX_MEASUREMENTS, chunk, line_buffer, &line_len);
        }

        
       /* {
            
            bool debug = false;
            min_dt = 999999.0;
            max_dt = 0.0;
            sum_dt = 0.0;
            count_dt = 0;

            struct timeval last_time, current_time;
            gettimeofday(&last_time, NULL);            

            // LookUp-Tabelle erstellen
            if (open_database(&db, "LookUpTabelle.db") != 0) 
                return 1;

            if (!create_lookup_table(db, "Messung1.csv"))
                return 1;

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

                // Sensorwert messen
                float sensorwert = -1.0f;
                while (sensorwert < 0)
                    sensorwert = read_sensor_value(serial_fd, chunk, line_buffer, &line_len);

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


                // Interpolation
                double interpolated_value = Interpolate_measurement(db, sensorwert, debug);
                if (isnan(interpolated_value)) {
                    printf("Interpolation fehlgeschlagen.\n");
                    continue;
                }

                // Abweichung
                double abweichung = interpolated_value - sensorwert;

                double Hoehe = 12.5;   //abstand Sensor zum Boden des profils
                if (interpolated_value > Hoehe) 
                {
                    interpolated_value = Hoehe;
                } //Vermeidung negativer Profilhöhe
                    double profilhoehe = Hoehe - interpolated_value;

                // In DB einfügen
                char temp[256];
                sprintf(temp,
                    "INSERT INTO Messung3 (Zeitstempel, Abstand_Sensor, interpolierter_Wert, Abweichung, ProfilHoehe) "
                    "VALUES (DATETIME('now'),%f,%f,%f,%f);",
                    sensorwert, interpolated_value, abweichung, profilhoehe);

                execute_sql(db, temp);

                // Tabelle in CSV schreiben
                execute_sql_csv("Messung3.csv", db,
                    "SELECT * FROM Messung3;");

                // Histogramm-Array vorbereiten
                int histogram[MAX_MEASUREMENTS];
                int length = 0;

                FILE *fp = fopen("Messung3.csv", "r");
                if (!fp) {
                    printf("CSV konnte nicht geöffnet werden!\n");
                    return 1;
                }

                char line[256];

                fgets(line, sizeof(line), fp);

                // CSV Zeilen einlesen
                while (fgets(line, sizeof(line), fp))
                {
                    int nr;
                    char timestamp[64];
                    float sensor, interpoliert, abw_csv, profilhoehe_csv;

                    if (sscanf(line, "%d,%[^,],%f,%f,%f,%f",
                            &nr, timestamp, &sensor, &interpoliert, &abw_csv, &profilhoehe_csv) == 6)
                    {
                        if (length >= MAX_MEASUREMENTS)
                            break;
                        
                        if (profilhoehe_csv < 0) 
                            {profilhoehe_csv = 0;}
                        int value = (int)round(profilhoehe_csv); 
                        if (value < 0) value = 0;

                        histogram[length++] = value;
                    }
                }

                fclose(fp);

                // Histogramm anzeigen
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

            
        }*/
       



        else if (strcmp(modus, "5") == 0) //Aufgabe 4
        {
            int rc = filter_run_from_csv("Messung3.csv", "Messung3_filtered.csv");
        }

            /*
        printf("\n--- FILTER-Modus gestartet ---\n");
       
    FILE *fp = fopen("Messung3.csv", "r");
    if (!fp) {
        printf("Fehler: Messung3.csv nicht gefunden!\nBitte zuerst AUTO-Modus ausführen.\n");
        continue;
    }

    size_t capacity = 1024;
    size_t n = 0;
    double *raw = malloc(capacity * sizeof(double));
    double *filtered = NULL;
    double *diff = NULL;
    if (!raw) {
        fprintf(stderr, "Speicherfehler\n");
        fclose(fp);
        continue;
    }

    char buf[2048];
    // Header überspringen falls vorhanden 
    if (!fgets(buf, sizeof(buf), fp)) {
        fprintf(stderr, "Datei leer oder Lesefehler\n");
        free(raw);
        fclose(fp);
        continue;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        // Zeilenlänge prüfen: falls länger als Puffer, überspringen
        size_t len = strlen(buf);
        if (len == sizeof(buf)-1 && buf[len-1] != '\n') {
            // zu lange Zeile: verwerfen Rest der Zeile
            int c;
            while ((c = fgetc(fp)) != EOF && c != '\n') {}
            continue;
        }

        // CR entfernen (Windows-Zeilenenden)
        if (len > 0 && buf[len-1] == '\n') buf[--len] = '\0';
        if (len > 0 && buf[len-1] == '\r') buf[--len] = '\0';

        // Tokenize CSV: nr,ts,sensor,interpoliert,abw 
        char *saveptr = NULL;
        char *tok = strtok_r(buf, ",", &saveptr); // nr 
        if (!tok) continue;
        tok = strtok_r(NULL, ",", &saveptr); // ts 
        if (!tok) continue;
        tok = strtok_r(NULL, ",", &saveptr); // sensor 
        if (!tok) continue;
        tok = strtok_r(NULL, ",", &saveptr); // interpoliert 
        if (!tok) continue;
        

        // strtod mit Fehlerprüfung 
        char *endptr;
        double interpoliert = strtod(tok, &endptr);
        if (endptr == tok) {
            // kein gültiger double 
            continue;
        }

        if (n >= capacity) {
            size_t newcap = capacity * 2;
            double *r2 = realloc(raw, newcap * sizeof(double));
            if (!r2) break;
            raw = r2;
            capacity = newcap;
        }
        raw[n++] = interpoliert;
    }
    fclose(fp);

    if (n == 0) {
        printf("Keine Werte eingelesen.\n");
        free(raw);
        continue;
    }

    // Speicher für Ergebnisse 
    filtered = malloc(n * sizeof(double));
    diff = malloc(n * sizeof(double));
    if (!filtered || !diff) {
        fprintf(stderr, "Speicherfehler (Ergebnisarrays)\n");
        free(raw); free(filtered); free(diff);
        continue;
    }

    if (n == 1) {
        filtered[0] = raw[0];
    } else {
        // Randbehandlung: 2-Punkt-Mittel an den Rändern 
        filtered[0] = (raw[0] + raw[1]) / 2.0;
        for (size_t i = 1; i + 1 < n; ++i) {
            filtered[i] = (raw[i-1] + raw[i] + raw[i+1]) / 3.0;
        }
        filtered[n-1] = (raw[n-2] + raw[n-1]) / 2.0;
    }

    for (size_t i = 0; i < n; ++i) diff[i] = raw[i] - filtered[i];

    FILE *out = fopen("Messung3_filtered.csv", "w");
    if (!out) {
        fprintf(stderr, "Fehler beim Schreiben der Output-CSV\n");
        free(raw); free(filtered); free(diff);
        continue;
    }
    fprintf(out, "raw,filtered,diff\n");
    for (size_t i = 0; i < n; ++i) {
        fprintf(out, "%.6f,%.6f,%.6f\n", raw[i], filtered[i], diff[i]);
    }
    fclose(out);

    printf("✔ %zu Werte gelesen. Datei Messung3_filtered.csv geschrieben\n", n);

    free(raw); free(filtered); free(diff);
*/
        
    
    
    else if (strcmp(modus, "6") == 0) //Aufgabe 5
        {
            //Aufgabe 5 programmieren
            
        } 
        
        
    
    else 
        {
            printf("Ungültiger Modus. Bitte erneut versuchen.\n");
            continue;
        }
       
        #if SIM 
            close(serial_fd);
        #endif
        sqlite3_close(db);
        
    }
    
    return 0;

}






