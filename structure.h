#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define Buf_size 24

typedef struct {
    int *arrayNodi; // array di nodi con archi entranti
    int numArchi; // numero di archi entranti
    int maxDim; // dimensione masssima dell'array
} inmap;

typedef struct {
    int N; // numero dei nodi del grafo
    int *out; // array con il numero di archi uscenti da ogni nodo
    inmap *in; // array con gli insiemi di archi entranti in ogni nodo
} grafo;

typedef struct {
    grafo *g;
    double *X; // vettore X
    double *X1; // vettore X1
    double *Y; // vettore Y
    double d; // damping factor
    double damping_factor; // damping factor calcolato
    int inizio; // inizio del thread
    int fine; // fine del
} datiThreads;

typedef struct {
    grafo *g;
    int *buffer;
    int *pcindex;
    bool *fineDati; // fine dati nel buffer
    pthread_mutex_t *mutex; // mutex
    sem_t *sem_free_slots; // semaforo
    sem_t *sem_data_items; // semaforo
    int taux; // numero thread ausiliari
    double d; // damping factor (ridichiarato)
    int maxiter; // numero massimo di iterazioni
    double eps; // tolleranza iteraizioni
    int *X; // vettore X (ridichiarato)
    int iter; // contatore iterazioni
} datiConsumatori;

typedef struct {
    pthread_mutex_t signal_mutex; // mutex
    bool continua; // se il programma deve continuare
    int iterazione; // iterazione
    double *vettoreX; // vettore X
    int N; // numero di nodi (ridichiarato)
} statoProgramma;

extern statoProgramma stato;
