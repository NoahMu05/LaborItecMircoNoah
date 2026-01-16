#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void strip_crlf(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) s[--len] = '\0';
}

int filter_run_from_csv(const char *in_path, const char *out_path) {
    if (!in_path || !out_path) return FILTER_ERR_FORMAT;

    printf("\n--- FILTER-Modus gestartet ---\n");

    FILE *fp = fopen(in_path, "r");
    if (!fp) {
        printf("Fehler: %s nicht gefunden!\nBitte zuerst AUTO-Modus ausführen.\n", in_path);
        return FILTER_ERR_IO;
    }

    size_t capacity = FILTER_INITIAL_CAPACITY;
    size_t n = 0;
    double *raw = malloc(capacity * sizeof(double));
    double *filtered = NULL;
    double *diff = NULL;
    if (!raw) {
        fprintf(stderr, "Speicherfehler\n");
        fclose(fp);
        return FILTER_ERR_MEM;
    }

    char buf[FILTER_LINE_BUFFER];
    if (!fgets(buf, sizeof(buf), fp)) {
        fprintf(stderr, "Datei leer oder Lesefehler\n");
        free(raw);
        fclose(fp);
        return FILTER_ERR_IO;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        size_t len = strlen(buf);
        if (len == sizeof(buf) - 1 && buf[len - 1] != '\n') {
            int c;
            while ((c = fgetc(fp)) != EOF && c != '\n') {}
            continue;
        }
        strip_crlf(buf);

        char *saveptr = NULL;
        char *tok = strtok_r(buf, ",", &saveptr);
        if (!tok) continue;
        tok = strtok_r(NULL, ",", &saveptr);
        if (!tok) continue;
        tok = strtok_r(NULL, ",", &saveptr);
        if (!tok) continue;
        tok = strtok_r(NULL, ",", &saveptr);
        if (!tok) continue;

        char *endptr;
        double interpoliert = strtod(tok, &endptr);
        if (endptr == tok) continue;

        if (n >= capacity) {
            size_t newcap = capacity * 2;
            double *r2 = realloc(raw, newcap * sizeof(double));
            if (!r2) break;
            raw = r2;
            capacity = newcap;
        }
        raw[n++] = interpoliert;
    }
    fclose(fp);

    if (n == 0) {
        printf("Keine Werte eingelesen.\n");
        free(raw);
        return FILTER_ERR_FORMAT;
    }

    filtered = malloc(n * sizeof(double));
    diff = malloc(n * sizeof(double));
    if (!filtered || !diff) {
        fprintf(stderr, "Speicherfehler (Ergebnisarrays)\n");
        free(raw); free(filtered); free(diff);
        return FILTER_ERR_MEM;
    }

    if (n == 1) {
        filtered[0] = raw[0];
    } else {
        filtered[0] = (raw[0] + raw[1]) / 2.0;
        for (size_t i = 1; i + 1 < n; ++i) {
            filtered[i] = (raw[i-1] + raw[i] + raw[i+1]) / 3.0;
        }
        filtered[n-1] = (raw[n-2] + raw[n-1]) / 2.0;
    }

    for (size_t i = 0; i < n; ++i) diff[i] = raw[i] - filtered[i];

    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Fehler beim Schreiben der Output-CSV\n");
        free(raw); free(filtered); free(diff);
        return FILTER_ERR_IO;
    }
    fprintf(out, "raw,filtered,diff\n");
    for (size_t i = 0; i < n; ++i) {
        fprintf(out, "%.6f,%.6f,%.6f\n", raw[i], filtered[i], diff[i]);
    }
    fclose(out);

    printf("✔ %zu Werte gelesen. Datei %s geschrieben\n", n, out_path);

    free(raw); free(filtered); free(diff);
    return FILTER_OK;
}