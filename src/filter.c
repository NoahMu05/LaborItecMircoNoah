//Filter: Mittelwertfilter (moving average) über 3 Werte
#include "filter.h"

void moving_average_3(const int *in, int n, double *out){
    if(n<=0) return;
    for(int i=0;i<n;i++){
        if(i==0) out[i] = (in[0] + (n>1?in[1]:in[0]))/2.0;
        else if(i==n-1) out[i] = (in[n-1] + in[n-2])/2.0;
        else out[i] = (in[i-1] + in[i] + in[i+1]) / 3.0;
    }
}