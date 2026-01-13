#ifndef INTERP_H
#define INTERP_H
// Build lookup from CSV and interpolate (double arrays)
int build_lookup_from_csv(const char *csvfile, double **sensor_vals, double **real_vals, int *count);
double interpolate_from_lookup(double sensor, double *sensor_vals, double *real_vals, int count);
void free_lookup(double *sensor_vals, double *real_vals);
#endif
