#ifndef INTERP_H
#define INTERP_H
// Build lookup from CSV and interpolate
int build_lookup_from_csv(const char *csvfile, int **sensor_vals, int **real_vals, int *count);
double interpolate_from_lookup(int sensor, int *sensor_vals, int *real_vals, int count);
void free_lookup(int *sensor_vals, int *real_vals);
#endif