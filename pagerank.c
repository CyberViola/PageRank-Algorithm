// PAGERANK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "structure.h"  
#include "xerrori.h"

int sizeBuffer;

// graph read
void graphRead(const char *fileInput, graph *g, int *numEdges) {
    FILE *file = fopen(fileInput, "r");
    if (file==NULL) {
        fileError("Error: input file opening");
    }

    char line[256];
    int r, c, n;
    bool dataLine = false; // to read the matrix
    *numEdges = 0;

    while (fgets(line, sizeof(line), file)!=NULL) {
        // ignore lines that start with %
        if (line[0]=='%' || line[0]=='\n') {
            continue;
        }

        for (char *v = line; *v; v++) {
            if (*v==',') *v=' ';
        }
        
        if (!dataLine) {
            // read the line containing the number of nodes and edges
            if (sscanf(line, "%d %d %d", &r, &c, &n)!=3) {
                inputError("Error: invalid input.\n");
            }
            dataLine = true;

            g->N = r; // graph nodes
            g->out = malloc(g->N*sizeof(int)); // outgoing edges
            g->in = malloc(g->N*sizeof(inmap)); // incoming edges
            if (g->out==NULL || g->in==NULL) {
                memoryError("Memory allocation error for g->out or g->in");
            }

            // nodes initialization
            for (int i=0; i<g->N; i++) {
                g->out[i] = 0;
                g->in[i].nodesArray = NULL;
                g->in[i].numEdges = 0;
                g->in[i].maxSize = 0;
            }
        } else {
            // read edges between nodes
            int i=0; // node 1
            int j=0; // node 2

            if (sscanf(line, "%d %d", &i, &j)!=2) {
                inputError("Error: invalid input.\n");
            }
            i--;
            j--;

            if (i<0 || i>=g->N || j<0 || j>=g->N) {
                inputError("Error: Invalid edges.\n");
            }

            if (i!=j) {  
                // add edge
                g->out[i]++;
                if (g->in[j].numEdges == g->in[j].maxSize) {
                    g->in[j].maxSize = (g->in[j].maxSize==0) ? 1 : g->in[j].maxSize*2;
                    g->in[j].nodesArray = realloc(g->in[j].nodesArray, g->in[j].maxSize*sizeof(int));
                    if (g->in[j].nodesArray==NULL) {
                        memoryError("Error realloc");
                    }
                }
                g->in[j].nodesArray[g->in[j].numEdges++]=i;
                (*numEdges)++;
            }
        }
    
    }
    fclose(file);
}

// threads to calculating pagerank
void *pagerankThreads(void *arg) {
    dataThreads *data = (dataThreads *)arg;
    graph *g = data->g;
    double *X = data->X;
    double *X1 = data->X1;
    double d = data->d;
    double damping_factor = data->damping_factor;

    for (int i=data->start; i<data->end; i++) {
        X1[i]=0.0;
        for (int j=0; j<g->in[i].numEdges; j++) {
            int incomingNode = g->in[i].nodesArray[j];
            X1[i] += d*X[incomingNode]/g->out[incomingNode];
        }
        X1[i] += damping_factor;
    }
    pthread_exit(NULL);
}

// calculate pagerank
double *pagerank(graph *g, double d, double eps, int maxiter, int taux, int *numiter) {
    int N = g->N;
    double *X = malloc(N * sizeof(double));
    double *X1 = malloc(N * sizeof(double));
    double *Y = malloc(N * sizeof(double));
    for (int i=0; i<N; i++) {
        X[i] = 1.0/N;
        X1[i] = 0.0;
        Y[i] = 0.0;
    }
    double damping_factor = (1.0-d)/N;

    int iter = 0;
    bool finished = false;

    while (!finished && iter<maxiter) {
        pthread_t threads[taux];
        dataThreads tdata[taux];
        int segment = (N+taux-1)/taux;

        // threads division to calculate pagerank
        for (int i=0; i<taux; i++) {
            tdata[i].g = g;
            tdata[i].X = X;
            tdata[i].X1 = X1;
            tdata[i].d = d;
            tdata[i].damping_factor = damping_factor;
            tdata[i].start = i*segment;
            tdata[i].end = (i+1)*segment;
            if (tdata[i].end>N) tdata[i].end = N;
            pthread_create(&threads[i], NULL, pagerankThreads, &tdata[i]);
        }

        // wait threads finish
        for (int i=0; i<taux; i++) {
            pthread_join(threads[i], NULL);
        }

        // deadend nodes handle
        double deadendNodes = 0.0;
        for (int i=0; i<N; i++) {
            if (g->out[i]==0) {
                deadendNodes += d*X[i]/N;
            }
        }
        for (int i=0; i<N; i++) {
            X1[i] += deadendNodes;
        }

        // calculate convergence error
        double error = 0.0;
        for (int i=0; i<N; i++) {
            error += fabs(X1[i]-X[i]);
        }
        if (error <= eps) {
            *numiter = iter+1;
            finished = true;
        }

        double *tmp = X;
        X = X1;
        X1 = tmp;

        iter++;
    }

    free(X1);
    free(Y);
    *numiter = iter;
    return X;
}

// consumers
void *consumers(void *arg) {
    dataConsumer *a = (dataConsumer *)arg;

    int nu; // node i with outgoing edge
    int ne; // node j with incoming edge
    int bufferIndex = 0;

    while (true) {
        sem_wait(a->sem_data_items);
        pthread_mutex_lock(a->mutex);
        if (*(a->pcindex)<0 || *(a->pcindex) >= sizeBuffer) {
            pthread_mutex_unlock(a->mutex);
            inputError("Error: invalid buffer index.\n");
        }

        // take nodes from the buffer
        nu = a->buffer[bufferIndex % sizeBuffer];
        bufferIndex = (bufferIndex+1) % sizeBuffer;
        ne = a->buffer[bufferIndex % sizeBuffer];
        bufferIndex = (bufferIndex+1) % sizeBuffer;
        pthread_mutex_unlock(a->mutex);
        sem_post(a->sem_free_slots);

        // check if data processing is finished
        if (*(a->endData)) break; 
        if (nu==-1 && ne==-1) break;

        if (nu<0 || nu>=a->g->N || ne<0 || ne>=a->g->N) {
            inputError("Error: Invalid node.");
        }

        if (nu!=ne) {
            a->g->out[nu]++;
            // add edge
            if (a->g->in[ne].numEdges == a->g->in[ne].maxSize) {
                a->g->in[ne].maxSize = (a->g->in[ne].maxSize==0) ? 1 : a->g->in[ne].maxSize*2;
                a->g->in[ne].nodesArray = realloc(a->g->in[ne].nodesArray, a->g->in[ne].maxSize*sizeof(int));
                if (a->g->in[ne].nodesArray==NULL) {
                    memoryError("Realloc error");
                }
            }
            a->g->in[ne].nodesArray[a->g->in[ne].numEdges++] = nu; 
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int K = 3; // show top K nodes
    int M = 100; // maximum number of iterations
    double D = 0.9; // damping factor
    double E = 1.e-7; // max error
    int T = 3; // max error
    const char *fileInput = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:m:d:e:t:")) != -1) {
        switch (opt) {
            case 'k':
                K = atoi(optarg);
                break;
            case 'm':
                M = atoi(optarg);
                break;
            case 'd':
                D = atof(optarg);
                break;
            case 'e':
                E = atof(optarg);
                break;
            case 't':
                T = atoi(optarg);
                break;
            default:
                fileError("Use: program name [-k K] [-m M] [-d D] [-e E] [-t T] input file.\n");
        }
    }

    if (optind >= argc) {
        fileError("Error: missing input file\n");
    }

    // graph read
    fileInput = argv[optind];
    graph g;
    int numEdges = 0;
    graphRead(fileInput, &g, &numEdges);

    // buffer and semaphores initialization
    sizeBuffer = numEdges*2;
    int *buffer = malloc(sizeBuffer*sizeof(int));

    if (buffer == NULL) {
        memoryError("Buffer allocation error.");
    }
    
    for (int i=0; i<sizeBuffer; i++) {
        buffer[i] = -2;
    }

    int pcindex = 0;
    pthread_t t[T];
    dataConsumer a[T];
    bool endData = false;
    pthread_mutex_t mutex;
    sem_t sem_free_slots, sem_data_items;
    pthread_mutex_init(&mutex, NULL);
    sem_init(&sem_free_slots, 0, sizeBuffer);
    sem_init(&sem_data_items, 0, 0);

    // create consumer threads
    for (int i=0; i<T; i++) {
        a[i].g = &g;
        a[i].buffer = buffer;
        a[i].pcindex = &pcindex;
        a[i].endData = &endData;
        a[i].mutex = &mutex;
        a[i].sem_free_slots = &sem_free_slots;
        a[i].sem_data_items = &sem_data_items;
        a[i].taux = T;
        a[i].d = D;
        a[i].maxIter = M;
        a[i].eps = E;
        pthread_create(&t[i], NULL, consumers, &a[i]);
    }

    // file opening
    FILE *file = fopen(fileInput, "r");
    if (file == NULL) {
        fileError("Error: input file opening.");
    }

    // read data
    int edges = 0;
    int ni, nj;
    while (fscanf(file, "%d %d", &ni, &nj)==2) {
        ni--;
        nj--;

        if (ni>=0 && ni<g.N && nj>=0 && nj<g.N) {
            sem_wait(&sem_free_slots);
            pthread_mutex_lock(&mutex);
            pcindex = (pcindex+1)%sizeBuffer;
            buffer[pcindex] = ni; // write outgoing node
            pcindex = (pcindex+1)%sizeBuffer;
            buffer[pcindex] = nj; // write incoming node
            pthread_mutex_unlock(&mutex);
            sem_post(&sem_data_items);
            edges++;
        } else {
            inputError("Error: Invalid edges.\n");
        }
    }

    // file closing
    fclose(file);

    // signal of data end
    for (int i=0; i<T; i++) {
        sem_wait(&sem_free_slots);
        pthread_mutex_lock(&mutex);
        buffer[pcindex % sizeBuffer] = -1;
        pcindex++;
        buffer[pcindex % sizeBuffer] = -1;
        pcindex++;
        pthread_mutex_unlock(&mutex);
        sem_post(&sem_data_items);
    }
    endData = true;

    // wait thread finish
    for (int i=0; i<T; i++) {
        pthread_join(t[i], NULL);
    }

    // calculate pagerank
    int numiter = 0;
    double *rank = pagerank(&g, D, E, M, T, &numiter);

    // results print
    printf("Number of nodes: %d\n", g.N);
    int deadendNodes = 0;
    for (int i=0; i<g.N; i++) {
        if (g.out[i]==0) {
            deadendNodes++;
        }
    }
    printf("Number of dead-end nodes: %d\n", deadendNodes);
    printf("Number of valid arcs: %d\n", numEdges);
    if (numiter==M) {
        printf("Did not converge after %d iterations\n", M);
    } else {
        printf("Converged after %d iterations\n", numiter);
    }

    // sum nodes rank
    double sumRanks = 0.0;
    for (int i=0; i<g.N; i++) {
        sumRanks += rank[i];
    }
    printf("Sum of ranks: %.4f\n", sumRanks);

    // print nodes with highest rank
    printf("Top %d nodes:\n", K);
    for (int i=1; i<=K && i<=g.N; i++) {
        int maxIndex = 0;
        double maxRank = rank[0];
        for (int j=1; j<g.N; j++) {
            if (rank[j] > maxRank) {
                maxRank = rank[j];
                maxIndex = j;
            }
        }
        printf("%d %.6f\n", maxIndex, rank[maxIndex]);
        rank[maxIndex] = -1;
    }

    // free memory and destroy semaphores and mutex
    free(buffer);
    for (int i=0; i<g.N; i++) {
        free(g.in[i].nodesArray);
    }
    free(g.in);
    free(g.out);
    free(rank);
    sem_destroy(&sem_free_slots);
    sem_destroy(&sem_data_items);
    return 0;
}