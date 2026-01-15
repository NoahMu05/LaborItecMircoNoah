#include "itec.h"

#include <stdio.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

int max_arr(int arr_lenght, int *arr) {

    int max = arr[0];   

    for (size_t i = 1; i < arr_lenght; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }

    return max;
}

int max_arr_struct(struct array_static *arr) {

    int max = arr->values[0];
    for (size_t i= 1; i < arr->lenght; i++) {
        if (arr->values[i] > max) {
            max = arr->values[i];
        }
    }

    return max;
}


int get_terminal_dim (struct winsize *w)
{
    //get terminal window size
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, w) == -1) {
        perror("ioctl");
        return 1;
    }

    return 0;
};

int print_hist(int arr_lenght, int *arr, int start, int end ) {
    int maxE = max_arr(arr_lenght, arr);

    for (int j = 0; j < maxE; j++) {
        for (int i = start; i < end; i++) 
        {
            if (arr[i]>=(maxE-j)){
                printf("#");
            }
            else
            {
                printf(" ");
            } 
        }
        printf("\n");
    }
    return 1;
}

size_t max_arr_i(int arr_lenght, int *arr) {
    
    size_t max_i = 0;
    int max = arr[0];

    for (int i = 1; i < arr_lenght; i++){
        if (arr[i]>max){
            max = arr[i];
            max_i = i;
        }
    }

    return max_i;

}

void swap (int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;    
}

int partition(int arr[], int low, int high)
{
    int pivot = arr[low];
    int i = low;
    int j = high;

    while (i<j)
    {
        while (arr[i] <= pivot && i <= high - 1)
        {
            i++;
        }

        while (arr[j] > pivot && j >= low + 1)
        {
            j--;
        }
        if (i < j) 
        {
            swap(&arr[i], &arr[j]);
            /* print_hist(high, arr, 0, high);
            usleep(500*1000);
            system("clear"); */
        }
        
    }
    swap(&arr[low], &arr[j]);
    return j;
}

void quickSort(int arr[], int low, int high)
{
    if (low < high) {

        int partitionIndex = partition(arr, low, high);

        quickSort(arr, low, partitionIndex - 1);
        quickSort(arr, partitionIndex + 1, high);
    }
}

void mergeSort(int arr[], int l, int r)
{
    if(l > r-1){
        return;
    }

    int m = l + (r - l) / 2;

    mergeSort(arr, l, m);
    mergeSort(arr, m + 1, r);

    merge(arr, l, m, r);
}

void merge(int arr[], int l, int m, int r)
{
    int i, j;
    int k;
    int n1 = m - l + 1;
    int n2 = r - m;

    int L[n1], R[n2];

    for (i = 0; i < n1; i++) //
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // Merge the temp arrays back
    // into arr[l..r]
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

int open_database(sqlite3 **db, const char *db_name) 
{
    int rc = sqlite3_open(db_name, db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(*db));
        return 1;
    }
    return 0;
}

int execute_sql(sqlite3 *db, const char *sql) 
{
    sqlite3_stmt *stmt;
    const char *tail;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    
    // Spaltenüberschriften ausgeben
    int cols = sqlite3_column_count(stmt);
    if (cols > 0) {
        for (int i = 0; i < cols; i++) {
            printf("%s", sqlite3_column_name(stmt, i));
            if (i < cols - 1) printf(" | ");
        }
        printf("\n");
        
        // Trennlinie
        for (int i = 0; i < cols; i++) {
            int len = strlen(sqlite3_column_name(stmt, i));
            for (int j = 0; j < len; j++) printf("-");
            if (i < cols - 1) printf("-+-");
        }
        printf("\n");
    }
    
    // Daten ausgeben
    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        for (int i = 0; i < cols; i++) {
            const unsigned char *text = sqlite3_column_text(stmt, i);
            printf("%s", text ? (const char*)text : "NULL");
            if (i < cols - 1) printf(" | ");
        }
        printf("\n");
    }
    
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

int execute_sql_csv(const char *filename, sqlite3 *db, const char *sql) 
{
    sqlite3_stmt *stmt;
    const char *tail;
    FILE *f = fopen(filename, "w");
    if (!f) return -1;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
    if (rc != SQLITE_OK) {
        fclose(f);
        return rc;
    }

    int cols = sqlite3_column_count(stmt);

    // Header
    for (int i = 0; i < cols; i++) {
        fprintf(f, "%s", sqlite3_column_name(stmt, i));
        if (i < cols - 1) fprintf(f, ",");
    }
    fprintf(f, "\n");

    // Rows
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int i = 0; i < cols; i++) {
            const unsigned char *text = sqlite3_column_text(stmt, i);
            fprintf(f, "%s", text ? (const char*)text : "");
            if (i < cols - 1) fprintf(f, ",");
        }
        fprintf(f, "\n");
    }

    fclose(f);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}


int configure_com_port(const char *port, int serial_port)
{

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Fehler beim Abrufen der Einstellungen: %s\n", strerror(errno));
        close(serial_port);
        return 1;
    }

    // Baudrate setzen
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // 8N1 Konfiguration
    tty.c_cflag &= ~PARENB;        // Keine Parität
    tty.c_cflag &= ~CSTOPB;        // 1 Stopbit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            // 8 Datenbits
   // tty.c_cflag &= ~CRTSCTS;       // Keine Hardware-Flowcontrol
    tty.c_cflag |= CREAD | CLOCAL; // Lesen aktivieren & lokaler Modus

    // Raw-Modus
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR);
    tty.c_oflag &= ~OPOST;

    // Timeout und minimale Anzahl an Bytes
    tty.c_cc[VTIME] = 10; // 1s Timeout
    tty.c_cc[VMIN]  = 10; // mindestens 10 Zeichen

    // Einstellungen übernehmen
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Fehler beim Setzen der Einstellungen: %s\n", strerror(errno));
        close(serial_port);
        return 1;
    }
}

int configure_serial(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return -1;
    }

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cc[VTIME] = 5; // 0.5s Timeout
    tty.c_cc[VMIN] = 0; //has to be 0 for non-blocking read for mac os

    tcflush(fd, TCIFLUSH);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr: Fehler beim Setzen der Einstellungen");
        return -1;
    }

    return 0;
}

float convert_to_sensor_val(const char *line) {
    char *endptr = NULL;
    errno = 0;
    float value = strtof(line, &endptr);

    if (!(errno == 0 && endptr != line)) {
        value = -1.0f;
        fprintf(stderr, "Warning: could not parse float from '%s' - Errno: %s\n", line, strerror(errno));
    }

    return value;
}


ssize_t read_sim(int serial_fd,  char* chunk, size_t chunk_len){

    static sim_data s_data = {32, 0, 
        {10.25f, 12.80f, 15.60f, 18.45f, 21.10f, 23.95f, 26.70f, 29.55f,
        32.30f, 35.15f, 37.90f, 40.75f, 43.50f, 46.35f, 49.10f, 51.95f,
        54.70f, 57.55f, 60.30f, 63.15f, 65.90f, 68.75f, 71.50f, 74.35f,
        77.10f, 79.95f, 82.70f, 85.55f, 88.30f, 91.15f, 93.90f, 99.60f}};

    //sleep for 1 sec
    sleep(1);

    //neuen Werte aus liste holen
    float value = s_data.data[s_data.index];

    //update index
    s_data.index = (++s_data.index)%s_data.length; 


    if (chunk_len == 0) {
        return 0;
    }

    int written = snprintf(chunk, chunk_len, "%f\n", value);
    if (written < 0) {
        return -1;
    }

    if ((size_t)written >= chunk_len) {
        written = (int)(chunk_len - 1);
    }

    return written;
}

int connect_to_sensor(char * tty_path){
    //Oeffnen des seriellen Ports

    int serial_fd = open(tty_path, OPEN_FLAGS);

    if (serial_fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", tty_path, strerror(errno));
        return -1;
    }

    //Konfiguration des seriellen Ports
    if (configure_serial(serial_fd) != 0) {
        fprintf(stderr, "Failed to configure %s: %s\n", tty_path, strerror(errno));
        close(serial_fd);
        return -1;
    }
    return serial_fd;
}

float read_sensor_value( int serial_fd, char *chunk, char *line_buffer, size_t *line_len) 
{ 
    #if SIM 
        ssize_t bytes_read = read(serial_fd, chunk, READ_CHUNK); 
    #else 
        ssize_t bytes_read = read_sim(serial_fd, chunk, READ_CHUNK); 
    #endif 

    if (bytes_read <= 0) 
        { 
            return -1.0f; // Fehler oder keine Daten 
        } 
    for (size_t i = 0; i < bytes_read; ++i) 
    { 
        char c = chunk[i]; 
        if (c == '\r') continue; 
        if (c == '\n') 
        { 
            if (*line_len > 0) 
            { 
                line_buffer[*line_len] = '\0'; 
                float value = convert_to_sensor_val(line_buffer); 
                *line_len = 0; 
                return value; 
            } continue; 
        } 
        if (*line_len + 1 >= 2 * READ_CHUNK) 
        { 
            *line_len = 0; 
            continue; 
        } 
        line_buffer[(*line_len)++] = c; 
    } 
    return -1.0f; // Noch kein kompletter Wert 
}

int lookup_lower(sqlite3 *db, double sensor, LookupEntry *out) 
{
    const char *sql = "SELECT Abstand_Sensor, Abstand_Real " "FROM LookUpTabelle " "WHERE Abstand_Sensor <= ? " "ORDER BY Abstand_Sensor DESC " "LIMIT 1;"; 
    sqlite3_stmt *stmt; 
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) 
        return 0; 
    sqlite3_bind_double(stmt, 1, sensor); 
    int result = 0; 
    if (sqlite3_step(stmt) == SQLITE_ROW) 
    { 
        out->sensor_raw = sqlite3_column_double(stmt, 0); 
        out->real_distance = sqlite3_column_double(stmt, 1); 
        result = 1; 
    } 
    sqlite3_finalize(stmt); 
    return result; 
}

int lookup_upper(sqlite3 *db, double sensor, LookupEntry *out)
{
    const char *sql =
        "SELECT Abstand_Sensor, Abstand_Real "
        "FROM LookUpTabelle "
        "WHERE Abstand_Sensor >= ? "
        "ORDER BY Abstand_Sensor ASC "
        "LIMIT 1;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return 0;

    sqlite3_bind_double(stmt, 1, sensor);

    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out->sensor_raw    = sqlite3_column_double(stmt, 0);
        out->real_distance = sqlite3_column_double(stmt, 1);
        found = 1;
    }

    sqlite3_finalize(stmt);
    return found;
}

int import_lookup_from_csv(sqlite3 *db, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Konnte %s nicht öffnen!\n", filename);
        return 0;
    }

    char line[256];

    // Erste Zeile: Header überspringen
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        printf("Datei %s ist leer!\n", filename);
        return 0;
    }

    while (fgets(line, sizeof(line), fp))
    {
        // Leere Zeilen oder Zeilen, die nicht mit einer Ziffer anfangen, ignorieren
        if (line[0] == '\n' || line[0] == '\r' || !isdigit((unsigned char)line[0]))
            continue;

        int messung_nr;
        double abstand_real, abstand_sensor, abweichung;

        // Format: Messung_Nr,Abstand_Real,Abstand_Sensor,Abweichung
        if (sscanf(line, "%d,%lf,%lf,%lf",
                   &messung_nr, &abstand_real, &abstand_sensor, &abweichung) == 4)
        {
            char temp[256];
            sprintf(temp,
                "INSERT INTO LookUpTabelle (Messung_Nr, Abstand_Real, Abstand_Sensor, Abweichung) "
                "VALUES (%d,%f,%f,%f);",
                messung_nr, abstand_real, abstand_sensor, abweichung);

            execute_sql(db, temp);
        }
        else {
            printf("Zeile konnte nicht geparst werden: %s", line);
        }
    }

    fclose(fp);
    return 1;
}



double Interpolate_measurement(sqlite3 *db, float sensorwert, bool debug)
{
    LookupEntry low, high;

    //
    // 1. Unteren Nachbarwert suchen
    //
    if (!lookup_lower(db, sensorwert, &low)) {
        if (debug)
            printf("Kein unterer Nachbarwert gefunden! Extrapoliere nach unten...\n");

        // Kleinster und zweitkleinster Wert holen
        const char *sql =
            "SELECT Abstand_Sensor, Abstand_Real "
            "FROM LookUpTabelle "
            "ORDER BY Abstand_Sensor ASC "
            "LIMIT 2;";

        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
            return NAN;

        // erster Step = kleinster Wert → high
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            high.sensor_raw    = sqlite3_column_double(stmt, 0);
            high.real_distance = sqlite3_column_double(stmt, 1);
        } else {
            sqlite3_finalize(stmt);
            return NAN;
        }

        // zweiter Step = zweitkleinster Wert → low
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            low.sensor_raw    = sqlite3_column_double(stmt, 0);
            low.real_distance = sqlite3_column_double(stmt, 1);
        } else {
            sqlite3_finalize(stmt);
            return NAN;
        }

        sqlite3_finalize(stmt);
    }
    //
    // 2. Oberen Nachbarwert suchen
    //
    else if (!lookup_upper(db, sensorwert, &high)) {
        if (debug)
            printf("Kein oberer Nachbarwert gefunden! Extrapoliere nach oben...\n");

        // Größter und zweitgrößter Wert holen
        const char *sql =
            "SELECT Abstand_Sensor, Abstand_Real "
            "FROM LookUpTabelle "
            "ORDER BY Abstand_Sensor DESC "
            "LIMIT 2;";

        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
            return NAN;

        // erster Step = größter Wert → low
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            low.sensor_raw    = sqlite3_column_double(stmt, 0);
            low.real_distance = sqlite3_column_double(stmt, 1);
        } else {
            sqlite3_finalize(stmt);
            return NAN;
        }

        // zweiter Step = zweitgrößter Wert → high
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            high.sensor_raw    = sqlite3_column_double(stmt, 0);
            high.real_distance = sqlite3_column_double(stmt, 1);
        } else {
            sqlite3_finalize(stmt);
            return NAN;
        }

        sqlite3_finalize(stmt);
    }

    //
    // Debug-Ausgabe
    //
    if (debug) {
        printf("LOW  : Sensor=%.2f  Real=%.2f\n", low.sensor_raw, low.real_distance);
        printf("HIGH : Sensor=%.2f  Real=%.2f\n", high.sensor_raw, high.real_distance);
    }

    //
    // 3. Interpolation / Extrapolation
    //
    double interpolated_value;

    if (high.sensor_raw == low.sensor_raw) {
        interpolated_value = low.real_distance;
    } else {
        interpolated_value =
            low.real_distance +
            (sensorwert - low.sensor_raw) *
            (high.real_distance - low.real_distance) /
            (high.sensor_raw - low.sensor_raw);
    }

    return interpolated_value;
}

int create_lookup_table(sqlite3 *db, const char *csv_filename)
{
    // Alte Tabelle löschen
    execute_sql(db, "DROP TABLE IF EXISTS LookUpTabelle;");

    // Neue Tabelle erstellen
    execute_sql(db,
        "CREATE TABLE IF NOT EXISTS LookUpTabelle ("
        "Messung_Nr INTEGER PRIMARY KEY, "
        "Abstand_Real DOUBLE, "
        "Abstand_Sensor DOUBLE, "
        "Abweichung DOUBLE);");

    // CSV importieren
    if (!import_lookup_from_csv(db, csv_filename)) {
        printf("Fehler beim Importieren der Lookup-Tabelle!\n");
        return 0;
    }

    return 1;
}







