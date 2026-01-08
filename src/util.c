#include "util.h"
#include <stdio.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

void ascii_histogram_update(int value, int width, int height){
    // simple vertical bar scaled to height
    int maxv = 300; // expected max distance for scaling; adjust if needed
    if(value < 0) value = 0;
    if(value > maxv) value = maxv;
    int h = (value * height) / maxv;
    for(int row = height; row >= 0; --row){
        for(int col = 0; col < width; ++col){
            if(row <= h) putchar('#'); else putchar(' ');
        }
        putchar('\n');
    }
}

void print_menu(){
    puts("=== Menü ===");
    puts("1 DA  - Manuelle Datenaufnahme (20 Messungen)");
    puts("2 TEST - Kalibrierung mit Lookup und Interpolation");
    puts("3 AUTO - Automatische Messung (~10s) mit ASCII Histogramm");
    puts("4 FILTER - Mittelwertfilter über 3 Werte");
    puts("5 FUN - Spiel (U-Boot)");
    puts("6 EXPORT - Exportiere DB als CSV (messung_x.csv)");
    puts("7 CLEAR - Lösche Messdaten");
    puts("0 EXIT");
    printf("Wähle Option: ");
    fflush(stdout);
}

void timestamp_now(char *buf, size_t len){
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm);
}

int read_int_stdin(const char *prompt){
    char line[128];
    int val = 0;
    printf("%s", prompt);
    if(!fgets(line, sizeof(line), stdin)) return 0;
    val = atoi(line);
    return val;
}

int nonblocking_getchar(){
    struct termios oldt, newt;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    int c = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    return c;
}

void sleep_ms(int ms){
    usleep(ms * 1000);
}