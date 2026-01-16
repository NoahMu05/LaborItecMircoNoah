#ifndef ITEC_H 
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

int max_arr(int arr_lenght, int *arr);

size_t max_arr_i(int arr_lenght, int *arr);

int max_arr_struct(struct array_static *arr);

int get_terminal_dim (struct winsize *w);

int print_hist(int arr_lenght, int *arr, int start, int end );

void swap (int *a, int *b);

int partition(int arr[], int low, int high);

void quickSort(int arr[], int low, int high);

void mergeSort(int arr[], int l, int r);

void merge(int arr[], int l, int m, int r);

//Datenbank öffnen
int open_database(sqlite3 **db, const char *db_name);

int execute_sql(sqlite3 *db, const char *sql) ;

int execute_sql_csv(const char *filename, sqlite3 *db, const char *sql);

int configure_com_port(const char *port, int serial_port);
int configure_serial(int fd);

float convert_to_sensor_val(const char *line);

ssize_t read_sim(int serial_fd,  char* chunk, size_t chunk_len);

int connect_to_sensor(char * tty_path);

float read_sensor_value( int serial_fd, char *chunk, char *line_buffer, size_t *line_len);

int lookup_lower(sqlite3 *db, double sensor, LookupEntry *out);

int lookup_upper(sqlite3 *db, double sensor, LookupEntry *out);

int import_lookup_from_csv(sqlite3 *db, const char *filename);

double Interpolate_measurement(sqlite3 *db, float sensorwert, bool debug);

int create_lookup_table(sqlite3 *db, const char *csv_filename);



#endif 
