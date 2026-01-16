#ifndef UTILS_H
#define UTILS_H

#include "itec.h"
#include <sys/ioctl.h>
#include <unistd.h>

// array / sort / histogram helpers
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

#endif // UTILS_H
