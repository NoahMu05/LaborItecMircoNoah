#include "itec.h"
#include "utils.h"

#include <stdio.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int max_arr(int arr_lenght, int *arr) {
    int max = arr[0];
    for (size_t i = 1; i < arr_lenght; i++) {
        if (arr[i] > max) max = arr[i];
    }
    return max;
}

int max_arr_struct(struct array_static *arr) {
    int max = arr->values[0];
    for (size_t i = 1; i < arr->lenght; i++) {
        if (arr->values[i] > max) max = arr->values[i];
    }
    return max;
}

size_t max_arr_i(int arr_lenght, int *arr) {
    size_t max_i = 0;
    int max = arr[0];
    for (int i = 1; i < arr_lenght; i++) {
        if (arr[i] > max) {
            max = arr[i];
            max_i = i;
        }
    }
    return max_i;
}

int get_terminal_dim (struct winsize *w)
{
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, w) == -1) {
        perror("ioctl");
        return 1;
    }
    return 0;
}

int print_hist(int arr_lenght, int *arr, int start, int end ) {
    int maxE = max_arr(arr_lenght, arr);
    for (int j = 0; j < maxE; j++) {
        for (int i = start; i < end; i++) {
            if (arr[i] >= (maxE - j)) printf("#");
            else printf(" ");
        }
        printf("\n");
    }
    return 1;
}

void swap (int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int partition(int arr[], int low, int high) {
    int pivot = arr[low];
    int i = low;
    int j = high;
    while (i < j) {
        while (arr[i] <= pivot && i <= high - 1) i++;
        while (arr[j] > pivot && j >= low + 1) j--;
        if (i < j) swap(&arr[i], &arr[j]);
    }
    swap(&arr[low], &arr[j]);
    return j;
}

void quickSort(int arr[], int low, int high) {
    if (low < high) {
        int partitionIndex = partition(arr, low, high);
        quickSort(arr, low, partitionIndex - 1);
        quickSort(arr, partitionIndex + 1, high);
    }
}

void mergeSort(int arr[], int l, int r) {
    if (l > r - 1) return;
    int m = l + (r - l) / 2;
    mergeSort(arr, l, m);
    mergeSort(arr, m + 1, r);
}

void merge(int arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
    int L[n1], R[n2];
    for (i = 0; i < n1; i++) L[i] = arr[l + i];
    for (j = 0; j < n2; j++) R[j] = arr[m + 1 + j];
    i = 0; j = 0; k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) { arr[k++] = L[i++]; }
        else { arr[k++] = R[j++]; }
    }
    while (i < n1) { arr[k++] = L[i++]; }
    while (j < n2) { arr[k++] = R[j++]; }
}