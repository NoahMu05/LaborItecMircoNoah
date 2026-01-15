#ifndef ITEC_H  //If not defined
#define ITEC_H

#include <stdlib.h>
#include <sys/ioctl.h>
#include <sqlite3.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ARR_LENGHT 10000
#define READ_CHUNK 64
#define SIM 1  //Set to 0 to enable simulation mode, 1 for real sensor

//Praeprozessor Makro fuer die Oeffnungsflags des seriellen Ports
#ifdef __APPLE__
#define OPEN_FLAGS O_NONBLOCK
#else
#define OPEN_FLAGS O_RDWR
#endif

struct array_static
{
    int lenght;
    int values[MAX_ARR_LENGHT];
};

typedef struct _sim_data {
    size_t length;
    size_t index;
    float data[32];
} sim_data;

typedef struct 
{ 
    double sensor_raw; 
    double real_distance; 
} LookupEntry;

//returns the maximum value of the array
int max_arr(int arr_lenght, int *arr);

//returns the index of the maximum value of the array
size_t max_arr_i(int arr_lenght, int *arr);

//returns the maximum value of the array structured
int max_arr_struct(struct array_static *arr);

//gets terminal dimensions
int get_terminal_dim (struct winsize *w);

//prints histogram to terminal
int print_hist(int arr_lenght, int *arr, int start, int end );

//swap 2 integers
void swap (int *a, int *b);

//partition for quick sort
int partition(int arr[], int low, int high);

//QuickSort
void quickSort(int arr[], int low, int high);

//MergeSort
void mergeSort(int arr[], int l, int r);

//merge for MergeSort
void merge(int arr[], int l, int m, int r);

//Datenbank öffnen
int open_database(sqlite3 **db, const char *db_name);

////In eine Datenbank schreiben und in die kommandozeile schreiben
int execute_sql(sqlite3 *db, const char *sql) ;

//In eine Datenbank schreiben und in eine .csv Datei schreiben
int execute_sql_csv(const char *filename, sqlite3 *db, const char *sql);

//Com Port vorbereiten
int configure_com_port(const char *port, int serial_port);

/**
 * Configures the serial port with the desired settings.
 * 
 * Important settings are:
 * 
 * - Baudrate: 115200
 * - Initial read timeout: 0.5s
 * - no min bytes for read (non-blocking)
 * 
 * @param fd The file descriptor of the opened serial port.
 * @returns 0 on success, -1 on failure.
 */
int configure_serial(int fd);

/**
 * Handles a complete line read from the serial port.
 * @param line The line to handle (null-terminated string).
 * @returns the distance as float in cm, or -1.0f on error.
 */
float convert_to_sensor_val(const char *line);

/**
 * Simulation of the sensor read
 * @return the number of bytes read
 */
ssize_t read_sim(int serial_fd,  char* chunk, size_t chunk_len);

//Connect to the sensor via serial port
int connect_to_sensor(char * tty_path);

//Read a sensor value from the serial port
float read_sensor_value( int serial_fd, char *chunk, char *line_buffer, size_t *line_len);

//Look up the lower neighbor in the lookup table
int lookup_lower(sqlite3 *db, double sensor, LookupEntry *out);

//Look up the upper neighbor in the lookup table
int lookup_upper(sqlite3 *db, double sensor, LookupEntry *out);

//Import lookup table from csv file
int import_lookup_from_csv(sqlite3 *db, const char *filename);

//Interpolate and store a measurement in messung2 table
double Interpolate_measurement(sqlite3 *db, float sensorwert, bool debug);

int create_lookup_table(sqlite3 *db, const char *csv_filename);



#endif //ITEC_H
