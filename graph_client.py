#!/usr/bin/env python3

import socket
import struct
import sys

def invioDati(file):
    with open(file, 'r') as f:
        # lettura righe
        while True:
            linea = f.readline()
            if linea.startswith('%'):
                # ignora il commento
                continue
            else:
                # prende i valori
                n, _, archiTot = map(int, linea.strip().split())
                # print(f"nodi: {n}, archi: {archiTot}") 
                break

        # connessione al server
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect(("127.0.0.1", 56646))

        # invio del numero di nodi e archi (2 interi)
        header = struct.pack('ii', n, archiTot)  # crea un header con i dati in binario
        client.sendall(header) # invio al server

        # lettura ed invio degli archi
        archiInviati = 0
        for linea in f:
            nu, ne = map(int, linea.strip().split())
            nodiArco = struct.pack('ii', nu, ne)
            client.sendall(nodiArco)
            archiInviati+=1
        
        # print(f"archi inviati: {archiInviati}")

        # chiusura connessione
        client.close()


if __name__ == "__main__":
    # lettura argomenti
    if len(sys.argv)!=2:
        print("Errore: numero di argomenti errato")
        sys.exit(1)

    fileGrafo = sys.argv[1]
    invioDati(fileGrafo)
