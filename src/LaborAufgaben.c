#include "itec.h"
#include "filter.h"
#include "automode.h"
#include "lookup.h"
#include "da.h"

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
            da_run(serial_fd, MAX_MEASUREMENTS, chunk, line_buffer, &line_len);
        } 

        else if (strcmp(modus, "3") == 0) //Aufgabe 2:LookUpTabelle erstellen
        {
            lookup_run(serial_fd, MAX_MEASUREMENTS, chunk, line_buffer, &line_len);  
        } 


        else if (strcmp(modus, "4") == 0) // Aufgabe 3:Auto Messung
        {
            automode_run(serial_fd, MAX_MEASUREMENTS, chunk, line_buffer, &line_len);
        }
    

        else if (strcmp(modus, "5") == 0) //Aufgabe 4
        {
            int rc = filter_run_from_csv("Messung3.csv", "Messung3_filtered.csv");
        }       
    
    
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






