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

#define Buf_size 10

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

// lettura del grafo
void lettura(const char *fileinput, grafo *g, int *numArchi) {
    FILE *file = fopen(fileinput, "r");
    if (file==NULL) {
        erroreFile("Errore apertura file");
    }

    char linea[100];
    int r, c, n;
    bool lineaDati = false; // per la lettura della matrice

    while (fgets(linea, sizeof(linea), file)!=NULL) {
        // ignora le linee che iniziano con %
        if (linea[0]=='%') {
            continue;
        } else {
            if (!lineaDati) {
                // lettura della linea contenente il numero di nodi ed archi
                if (sscanf(linea, "%d %d %d", &r, &c, &n)!=3) {
                    erroreInput("Errore: Input non valido.\n");
                }
                lineaDati = true;

                g->N = r; // nodi nel grafo
                g->out = malloc(g->N*sizeof(int)); // archi uscenti
                g->in = malloc(g->N*sizeof(inmap)); // archi entranti
                if (g->out==NULL || g->in==NULL) {
                    erroreMemoria("Errore di allocazione memoria per g->out o g->in");
                }

                // per ogni nodo si inizializza tutto
                for (int i=0; i<g->N; i++) {
                    g->out[i] = 0;
                    g->in[i].arrayNodi = NULL;
                    g->in[i].numArchi = 0;
                    g->in[i].maxDim = 0;
                }
            } else {
                // lettura degli archi tra i nodi
                int i = 0; // nodo 1
                int j = 0; // nodo 2
                if (sscanf(linea, "%d %d", &i, &j)!=2) {
                    erroreInput("Errore: Input non valido.\n");
                }
                i--;
                j--;
                if (i<0 || i>=g->N || j<0 || j>=g->N) {
                    erroreInput("Errore: Archi non validi.\n");
                }

                if (i!=j) {  
                    // controllo archi ripetuti
                    bool doppio = false;
                    for (int k=0; k< g->in[j].numArchi; k++) {
                        if (g->in[j].arrayNodi[k]==i) {
                            doppio = true;
                            break;
                        }
                    }
                    if (!doppio) {
                        // controllo nuovi archi
                        g->out[i]++;
                        if (g->in[j].numArchi == g->in[j].maxDim) {
                            g->in[j].maxDim = (g->in[j].maxDim==0) ? 1 : g->in[j].maxDim*2;
                            g->in[j].arrayNodi = realloc(g->in[j].arrayNodi, g->in[j].maxDim*sizeof(int));
                            if (g->in[j].arrayNodi==NULL) {
                                erroreMemoria("Errore realloc");
                            }
                        }
                        g->in[j].arrayNodi[g->in[j].numArchi++]=i;
                        (*numArchi)++;
                    }
                }
            }
        }
    }
    fclose(file);
}

// threads per calcolo pagerank
void *threadsPageRank(void *arg) {
    datiThreads *dato = (datiThreads *)arg;
    grafo *g = dato->g;
    double *X = dato->X;
    double *X1 = dato->X1;
    double d = dato->d;
    double damping_factor = dato->damping_factor;

    for (int i=dato->inizio; i<dato->fine; i++) {
        X1[i]=0.0;
        for (int j=0; j< g->in[i].numArchi; j++) {
            int nodoEntrante = g->in[i].arrayNodi[j];
            X1[i] += d*X[nodoEntrante]/g->out[nodoEntrante];
        }
        X1[i] += damping_factor;
    }
    pthread_exit(NULL);
}

// calcolo pagerank
double *pagerank(grafo *g, double d, double eps, int maxiter, int taux, int *numiter) {
    int N = g->N;
    double *X = malloc(N * sizeof(double));
    double *X1 = malloc(N * sizeof(double));
    double *Y = malloc(N * sizeof(double));
    for (int i = 0; i < N; i++) {
        X[i] = 1.0 / N;
        X1[i] = 0.0;
        Y[i] = 0.0;
    }
    double damping_factor = (1.0-d)/N;

    int iter = 0;
    bool finito = false;

    while (!finito && iter<maxiter) {
        pthread_t threads[taux];
        datiThreads tdata[taux];
        int segmento = (N+taux-1)/taux;

        // suddivisione threads per calcolare pagerank
        for (int i=0; i<taux; i++) {
            tdata[i].g = g;
            tdata[i].X = X;
            tdata[i].X1 = X1;
            tdata[i].d = d;
            tdata[i].damping_factor = damping_factor;
            tdata[i].inizio = i*segmento;
            tdata[i].fine = (i+1)*segmento;
            if (tdata[i].fine>N) tdata[i].fine = N;
            pthread_create(&threads[i], NULL, threadsPageRank, &tdata[i]);
        }

        // attesa terminazione threads
        for (int i=0; i<taux; i++) {
            pthread_join(threads[i], NULL);
        }

        // gestione nodi deadend
        double nodiDeadend = 0.0;
        for (int i=0; i<N; i++) {
            if (g->out[i]==0) {
                nodiDeadend += d*X[i]/N;
            }
        }
        for (int i=0; i<N; i++) {
            X1[i] += nodiDeadend;
        }

        // calcolo errore per convergenza
        double error = 0.0;
        for (int i=0; i<N; i++) {
            error += fabs(X1[i]-X[i]);
        }
        if (error <= eps) {
            *numiter = iter+1;
            finito = true;
        }

        for (int i=0; i<N; i++) {
            X[i] = X1[i];
            stato.vettoreX[i] = X[i];
        }

        iter++;

        // aggiornamento variabili per segnale
        pthread_mutex_lock(&stato.signal_mutex);
        stato.iterazione = iter;
        memcpy(stato.vettoreX, X, N*sizeof(double));
        pthread_mutex_unlock(&stato.signal_mutex);
    }

    free(X1);
    free(Y);
    *numiter = iter;
    return X;
}

// consumatori
void *consumatori(void *arg) {
    datiConsumatori *a = (datiConsumatori *)arg;

    int nu; // nodo i con arco uscente
    int ne; // nodo j con arco entrante

    while (true) {
        sem_wait(a->sem_data_items);
        pthread_mutex_lock(a->mutex);
        if (*(a->pcindex)<0 || *(a->pcindex) >= Buf_size*2) {
            pthread_mutex_unlock(a->mutex);
            erroreInput("Errore: Indice del buffer non valido.\n");
        }
        // prende i nodi dal buffer
        nu = a->buffer[*(a->pcindex)%Buf_size*2];
        *(a->pcindex) = (*(a->pcindex)+1)%(Buf_size*2);
        ne = a->buffer[(*(a->pcindex)+1)%Buf_size*2];
        *(a->pcindex) = (*(a->pcindex)+1)%(Buf_size*2);
        pthread_mutex_unlock(a->mutex);
        sem_post(a->sem_free_slots);

        // controlla che siano finiti i dati da elaborare
        if (*(a->fineDati)) break; 
        if (nu==-1 && ne==-1) break;
        if (nu<0 || nu>=a->g->N || ne<0 || ne>=a->g->N) {
            erroreInput("Errore: Nodo non valido.");
        }
        if (nu!=ne) {
            a->g->out[nu]++;
            // verifica la presenza dell'arco
            bool presente = false;
            for (int i=0; i<(a->g->in[ne].numArchi); i++) {
                if (a->g->in[ne].arrayNodi[i]==nu) {
                    presente = true;
                    break;
                }
            }
            if (!presente) { 
                // aggiunge lparco se non presente
                if (a->g->in[ne].numArchi == a->g->in[ne].maxDim) {
                    a->g->in[ne].maxDim = (a->g->in[ne].maxDim==0) ? 1 : a->g->in[ne].maxDim*2;
                    a->g->in[ne].arrayNodi = realloc(a->g->in[ne].arrayNodi, a->g->in[ne].maxDim*sizeof(int));
                    if (a->g->in[ne].arrayNodi==NULL) {
                        erroreMemoria("Errore realloc");
                    }
                }
                a->g->in[ne].arrayNodi[a->g->in[ne].numArchi++] = nu; 
            }
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
    const char *fileinput = NULL;

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
                erroreFile("Utilizzo: nome programma [-k K] [-m M] [-d D] [-e E] [-t T] file input.\n");
        }
    }

    if (optind >= argc) {
        erroreFile("File input assente\n");
    }

    // lettura del grafo
    fileinput = argv[optind];
    grafo g;
    int numArchi = 0;
    lettura(fileinput, &g, &numArchi);

    // inizializzazione buffer e semafori
    int pcindex = 0;
    int buffer[Buf_size*2];
    pthread_t t[T];
    datiConsumatori a[T];
    bool fineDati = false;
    pthread_mutex_t mutex;
    sem_t sem_free_slots, sem_data_items;
    pthread_mutex_init(&mutex, NULL);
    sem_init(&sem_free_slots, 0, Buf_size);
    sem_init(&sem_data_items, 0, 0);

    // inizializzaizione il gestore dei segnali
    pthread_mutex_init(&stato.signal_mutex, NULL);
    stato.vettoreX = malloc(g.N*sizeof(double));
    if (signal(SIGUSR1, gestoreSegnale)==SIG_ERR) {
        erroreSegnale("Errore nella gestione del segnale.");
    }

    // creazione threads consumatori
    for (int i=0; i<T; i++) {
        a[i].g = &g;
        a[i].buffer = buffer;
        a[i].pcindex = &pcindex;
        a[i].fineDati = &fineDati;
        a[i].mutex = &mutex;
        a[i].sem_free_slots = &sem_free_slots;
        a[i].sem_data_items = &sem_data_items;
        a[i].taux = T;
        a[i].d = D;
        a[i].maxiter = M;
        a[i].eps = E;
        pthread_create(&t[i], NULL, consumatori, &a[i]);
    }

    // apertura file
    FILE *file = fopen(fileinput, "r");
    if (file == NULL) {
        erroreFile("Errore apertura file.");
    }

    // lettura dati
    int archi = 0;
    int ni, nj;
    while (fscanf(file, "%d %d", &ni, &nj)==2) {
        ni--;
        nj--;
        if (ni>=0 && ni<g.N && nj>=0 && nj<g.N) {
            sem_wait(&sem_free_slots);
            pthread_mutex_lock(&mutex);
            buffer[pcindex % (Buf_size*2)] = ni;
            pcindex++;
            buffer[pcindex % (Buf_size*2)] = nj;
            pcindex++;
            pthread_mutex_unlock(&mutex);
            sem_post(&sem_data_items);
            archi++;
        } else {
            erroreInput("Errore: archi non validi.\n");
        }
    }

    // chiusura file
    fclose(file);

    // segnale della fine dei dati
    for (int i=0; i<T; i++) {
        sem_wait(&sem_free_slots);
        pthread_mutex_lock(&mutex);
        buffer[pcindex++ % (Buf_size*2)] = -1;
        buffer[pcindex++ % (Buf_size*2)] = -1;
        pthread_mutex_unlock(&mutex);
        sem_post(&sem_data_items);
    }
    fineDati = true;

    // attesa terminazioe threads
    for (int i=0; i<T; i++) {
        pthread_join(t[i], NULL);
    }

    // calcolo pagerank
    int numiter = 0;
    double *rank = pagerank(&g, D, E, M, T, &numiter);

    // stampa dei risultati
    printf("Number of nodes: %d\n", g.N);
    int nodiDeadend = 0;
    for (int i=0; i<g.N; i++) {
        if (g.out[i]==0) {
            nodiDeadend++;
        }
    }
    printf("Number of dead-end nodes: %d\n", nodiDeadend);
    printf("Number of valid arcs: %d\n", numArchi);
    if (numiter==M) {
        printf("Did not converge after %d iterations\n", M);
    } else {
        printf("Converged after %d iterations\n", numiter);
    }

    // somma rank nodi
    double sumRanks = 0.0;
    for (int i=0; i<g.N; i++) {
        sumRanks += rank[i];
    }
    printf("Sum of ranks: %.4f\n", sumRanks);

    // stampa dei nodi con rank più alto
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

    // libera memoria e distruzione semafori e mutex
    for (int i=0; i<g.N; i++) {
        free(g.in[i].arrayNodi);
    }
    free(g.in);
    free(g.out);
    free(rank);
    free(stato.vettoreX);
    pthread_mutex_destroy(&stato.signal_mutex);
    sem_destroy(&sem_free_slots);
    sem_destroy(&sem_data_items);
    return 0;
}