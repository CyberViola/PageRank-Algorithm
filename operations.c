#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include "operations.h"

statoProgramma stato;

// gestione del segnale 
void gestoreSegnale(int s) {
    if (s==SIGUSR1) {
        pthread_mutex_lock(&stato.signal_mutex);
        int nodoMaxPR = 0;
        double valorePR = stato.vettoreX[0];
        // ricerca nodo con valore massimo di pagerank
        for (int i=1; i<stato.N; i++) {
            if (stato.vettoreX[i]>valorePR) {
                valorePR = stato.vettoreX[i];
                nodoMaxPR = i;
            }
        }
        fprintf(stderr, "Iterazione corrente: %d\n", stato.iterazione);
        fprintf(stderr, "Nodo con PageRank massimo: %d\n", nodoMaxPR);
        fprintf(stderr, "Valore PageRank: %.6f\n", valorePR);
        pthread_mutex_unlock(&stato.signal_mutex);
    }
}