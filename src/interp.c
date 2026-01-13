//Interpolation (double-Version)
#include "interp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int build_lookup_from_csv(const char *csvfile, double **sensor_vals, double **real_vals, int *count){
    FILE *f = fopen(csvfile, "r");
    if(!f) return -1;
    char line[512];
    // skip header
    if(!fgets(line, sizeof(line), f)){ fclose(f); return -1; }
    int capacity = 64;
    double *s = malloc(sizeof(double)*capacity);
    double *r = malloc(sizeof(double)*capacity);
    int n = 0;
    while(fgets(line, sizeof(line), f)){
        // expected: id,real_cm,sensor_raw,diff,ts
        char *tok = strtok(line, ",");
        if(!tok) continue;
        tok = strtok(NULL, ","); // real_cm
        if(!tok) continue;
        double real = atof(tok);
        tok = strtok(NULL, ","); // sensor_raw
        if(!tok) continue;
        double sensor = atof(tok);
        if(n >= capacity){
            capacity *= 2;
            s = realloc(s, sizeof(double)*capacity);
            r = realloc(r, sizeof(double)*capacity);
        }
        s[n] = sensor;
        r[n] = real;
        n++;
    }
    fclose(f);
    *sensor_vals = s;
    *real_vals = r;
    *count = n;
    return 0;
}

static int find_interval(double sensor, double *s, int n){
    if(n < 2) return -1;
    if(s[0] <= s[n-1]){
        for(int i=0;i<n-1;i++){
            if(sensor >= s[i] && sensor <= s[i+1]) return i;
        }
    } else {
        for(int i=0;i<n-1;i++){
            if(sensor <= s[i] && sensor >= s[i+1]) return i;
        }
    }
    return -1;
}

double interpolate_from_lookup(double sensor, double *sensor_vals, double *real_vals, int count){
    if(count == 0) return sensor;
    if(count == 1) return real_vals[0];
    int idx = find_interval(sensor, sensor_vals, count);
    if(idx == -1){
        // outside range: linear extrapolate using nearest two
        if(sensor < sensor_vals[0]){
            double x0 = sensor_vals[0], x1 = sensor_vals[1];
            double y0 = real_vals[0], y1 = real_vals[1];
            if(x1==x0) return y0;
            return y0 + (sensor - x0)*(y1 - y0)/(x1 - x0);
        } else {
            double x0 = sensor_vals[count-2], x1 = sensor_vals[count-1];
            double y0 = real_vals[count-2], y1 = real_vals[count-1];
            if(x1==x0) return y1;
            return y0 + (sensor - x0)*(y1 - y0)/(x1 - x0);
        }
    } else {
        double x0 = sensor_vals[idx], x1 = sensor_vals[idx+1];
        double y0 = real_vals[idx], y1 = real_vals[idx+1];
        if(x1==x0) return y0;
        return y0 + (sensor - x0)*(y1 - y0)/(x1 - x0);
    }
}

void free_lookup(double *sensor_vals, double *real_vals){
    if(sensor_vals) free(sensor_vals);
    if(real_vals) free(real_vals);
}
