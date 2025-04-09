#include "xerrori.h"

void fileError(const char *testo) {
    fprintf(stderr, "Errore file: %s\n", testo);
    exit(EXIT_FAILURE);
}

void inputError(const char *testo) {
    fprintf(stderr, "Errore input: %s\n", testo);
    exit(EXIT_FAILURE);
}

void memoryError(const char *testo) {
    fprintf(stderr, "Errore memoria: %s\n", testo);
    exit(EXIT_FAILURE);
}

