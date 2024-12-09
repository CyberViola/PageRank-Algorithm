# PageRank - Progetto Laboratorio 2
Il progetto consiste nell'implementazione dell'algoritmo di Google PageRank in C, con anche un server ed un client in Python per l'esecuzione di PageRank. (Linux)

## PageRank (C)
L'algoritmo [PageRank](https://it.wikipedia.org/wiki/PageRank) è implementato utilizzando il modello produttore-consumatore ed un approccio multithread.

Nelle struct viene rappresentato il grafo e tutto ciò che serve per il funzionamento del codice.
Si utilizza un modello produttore-consumatore in cui il main produce i threads consumatori, e questi in parallelo svolgono il calcolo del Pagerank sudduvidendo il loro lavoro in segmenti di nodi;
vengono utilizzati semafori e mutex per poi gestire il lavoro dei threads.

## Server & Client (Python)

Il server ed il client comunicano tra di loro allo scopo di eseguire il calcolo del PageRank, principalmente il client si occupa dei dati, mentre il server delle operazioni.

Client: legge i dati del grafo e li passa al server tramite un header, generando e facendo lavorare un numero di thread ausiliari pari al numero di file che gli vengono passati. <br>Server: riceve i dati dal client tramite header, e ne gestisce la connessione tramite threads, cerca gli archi invalidi, li elimina, e si crea un file temporaneo con tutto il grafo senza gli archi invalidi su cui poi esegue PageRank, stampando le informazioni su un file di log.  (Nota: eseguire il comando `chmod +x graph_server.py graph_client.py` per permessi di esecuzione)
