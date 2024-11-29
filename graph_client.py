#!/usr/bin/env python3

import socket
import struct
import sys
import threading

def invioDati(file):
    with open(file, 'r') as f:
        # lettura righe
        while True:
            linea = f.readline()
            if linea.startswith('%'):
                continue
            else:
                n, _, archiTot = map(int, linea.strip().split())
                break

        # connessione al server
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect(("127.0.0.1", 56646))

        # invio del numero di nodi e archi
        header = struct.pack('ii', n, archiTot)
        client.sendall(header)

        # invio del nome del file
        fileNameEncoded = file.encode()
        fileNameLen = struct.pack('I', len(fileNameEncoded))
        client.sendall(fileNameLen)
        client.sendall(fileNameEncoded)

        # lettura ed invio degli archi
        archiInviati = 0
        for linea in f:
            nu, ne = map(int, linea.strip().split())
            nodiArco = struct.pack('ii', nu, ne)
            client.sendall(nodiArco)
            archiInviati += 1

        # attesa della risposta dal server
        output = client.recv(4096).decode()  # ricezione dell'output dal server
        print(f"{file} {output}")  # stampa l'output con il nome del file

        # chiusura connessione
        client.close()
        print(f"{file} Bye")

def main():
    # lettura argomenti
    if len(sys.argv) < 2:
        print("Errore: numero di argomenti errato")
        sys.exit(1)

    # creazione di un thread per ogni file
    threads = []
    for fileGrafo in sys.argv[1:]:
        thread = threading.Thread(target=invioDati, args=(fileGrafo,))
        threads.append(thread)
        thread.start() # avvio threads

    for thread in threads:
        thread.join()  # attende che tutti i thread finiscano

if __name__ == "__main__":
    main()

# in caso di errore con comando ./graph_client.py 21archi.mtx rimandarlo una seconda volta
