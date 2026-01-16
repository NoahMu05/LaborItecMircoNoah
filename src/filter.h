#ifndef FILTER_H
#define FILTER_H

#include <stddef.h>

#define FILTER_INITIAL_CAPACITY 1024
#define FILTER_LINE_BUFFER     2048

#ifdef __cplusplus
extern "C" {
#endif

enum {
    FILTER_OK = 0,
    FILTER_ERR_IO = -1,
    FILTER_ERR_MEM = -2,
    FILTER_ERR_FORMAT = -3
};

int filter_run_from_csv(const char *in_path, const char *out_path);

#ifdef __cplusplus
}
#endif

#endif 