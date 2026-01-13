#include "filter.h"
#include <stdlib.h>

// Simple Mittelwertfilter über 3 Werte.
// Für Randpunkte werden die Originalwerte beibehalten.
void moving_average_3(const double *in, int n, double *out){
    if(n <= 0) return;
    if(n == 1){ out[0] = in[0]; return; }
    for(int i=0;i<n;i++){
        if(i == 0 || i == n-1){
            out[i] = in[i];
        } else {
            out[i] = (in[i-1] + in[i] + in[i+1]) / 3.0;
        }
    }
}
