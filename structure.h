#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    int *nodesArray; // array of nodes with incoming edges
    int numEdges; // number of incoming edges
    int maxSize; // maximum size of the array
} inmap;

typedef struct {
    int N; // number of nodes in the graph
    int *out; // array with the number of outgoing edges from each node
    inmap *in; // array with the sets of incoming edges for each node
} graph;

typedef struct {
    graph *g;
    double *X; // vector X
    double *X1; // vector X1
    double *Y; // vector Y
    double d; // damping factor
    double damping_factor; // calculated damping factor
    int start; // thread start
    int end; // thread end
} dataThreads;

typedef struct {
    graph *g;
    int *buffer;
    int *pcindex;
    bool *endData; // end of data in the buffer
    pthread_mutex_t *mutex; // mutex
    sem_t *sem_free_slots; // semaphore
    sem_t *sem_data_items; // semaphore
    int taux; // number of auxiliary threads
    double d; // damping factor (redeclared)
    int maxIter; // maximum number of iterations
    double eps; // tolerance for iterations
    int *X; // vector X (redeclared)
    int iter; // iteration counter
} dataConsumer;

