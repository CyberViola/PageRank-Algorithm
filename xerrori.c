#include "xerrori.h"

void erroreFile(const char *testo) {
    fprintf(stderr, "Errore file: %s\n", testo);
    exit(EXIT_FAILURE);
}

void erroreInput(const char *testo) {
    fprintf(stderr, "Errore input: %s\n", testo);
    exit(EXIT_FAILURE);
}

void erroreMemoria(const char *testo) {
    fprintf(stderr, "Errore memoria: %s\n", testo);
    exit(EXIT_FAILURE);
}

void erroreSegnale(const char *testo) {
    fprintf(stderr, "Errore segnale: %s\n", testo);
    exit(EXIT_FAILURE);
}
