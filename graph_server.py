#!/usr/bin/env python3

import socket
import threading
import os
import tempfile
import subprocess
import struct
import signal
import sys

# flag per la chiusura del server
flagChiusura = threading.Event()

def operazioniServer(socketClient):
    # connessine e comunicazione col client
    # print("client connesso")
    try:
        # lettura numero nodi e archi
        header = socketClient.recv(8)  # 8 bytes (4 n e 4 archiTot)
        n, archiTot = struct.unpack('ii', header) # ottenimento dei dati dall'header

        # print(f"nodi: {n}, archi: {archiTot}")

        if n<=0 or archiTot<=0:
            print("Numero invalido di nodi o archi")
            return

        # creazione di un file temporaneo per memorizzare il grafo
        tmpFilePath = tempfile.mktemp(suffix='.mtx')
        archiValidi=0
        with open(tmpFilePath, 'w') as tmpFile:
            tmpFile.write("% File temporaneo\n")  
            tmpFile.write(f"{n} {n} 0\n") # inizia a scrivere interi nel file

            #  lettura degli archi
            archiRicevuti=0
            archiValidi=0
            while archiRicevuti < archiTot:
                nodiArco = socketClient.recv(8) # 8 byes (un arco è tra 2 nodi)
                if not nodiArco:
                    break

                # ottenimento dei dati sugli archi
                nu, ne = struct.unpack('ii', nodiArco)

                # verifica e scrittura degli archi validi
                if 1<=nu<=n and 1<=ne<= n:
                    tmpFile.write(f"{nu} {ne}\n")
                    archiValidi += 1
                else:
                    continue
                    # print(f"arco invalido {nu} {ne} eliminato.")
                
                archiRicevuti += 1

            # print(f"archi ricevuti: {archiRicevuti}, archi validi: {archiValidi}")

        # aggiornamento degli archi validi nel file temporaneo
        if archiValidi>0:
            with open(tmpFilePath, 'r+') as tmpFile:
                linee = tmpFile.readlines() # lettura delle linee
                for i, linea in enumerate(linee):
                    if not linea.startswith('%'):
                        # aggiornamento del numero di arrchi validi
                        linee[i] = f"{n} {n} {archiValidi}\n"
                        break
                # aggiorna il file (riscrivendo il contenuto)
                tmpFile.seek(0)
                tmpFile.writelines(linee)
                tmpFile.truncate()  # rimuove righe in accesso

        # stampa file temporaneo
        # with open(tmpFilePath, 'r') as tmpFile:
            # tmpFileContenuto = tmpFile.read()
            # print(f"Contenuto del file temporaneo:\n{tmpFileContenuto}")

        # esecuzione pagerank
        if archiValidi>0:
            risultato = subprocess.run(
                ["./pagerank", tmpFilePath],
                capture_output=True,
                text=True
            )

            if risultato.returncode == 0:
                print(f"Exit code: {risultato.returncode}")
                print(f"{risultato.stdout}") # stampa output
            else:
                print(f"{risultato.stderr}") # stampa errori
        else:
            print("Nessun arco valido")

        #  invio dell'output al client
        if archiValidi>0:
            with open(tmpFilePath, 'r') as tmpFile:
                output = tmpFile.read()
                socketClient.sendall(output.encode())

    except Exception as errore:
        print(f"Error: {errore}")
    finally:
        # chisura del socket client
        socketClient.close()
        print("Bye")

def segnale(sig, frame):
    flagChiusura.set()
    # print("Segnale ricevuto")

def main():
    global flagChiusura

    signal.signal(signal.SIGINT, segnale)

    # configurazione server
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(("127.0.0.1", 56646))
    server.listen(5)
    print("Server in ascolto su 127.0.0.1:56646")

    # gestione connessioni
    while not flagChiusura.is_set():
        try:
            server.settimeout(1.0)  # timeout per evitare che si blocchi con accept()
            socketClient2, addr = server.accept() # accetta connesione dal client
            threadClient = threading.Thread(target=operazioniServer, args=(socketClient2,)) # turead per gestire il client
            threadClient.start()
        except socket.timeout:
            continue
        except Exception as errore:
            print(f"Error: {errore}")
            break

    # chiusura server
    server.close()
    print("Bye dal server")

if __name__ == "__main__":
    main()


# note:
# pkill a volte da problemi col prefisso ./
# dopo aver eseguito pkill premere invio per terminare
