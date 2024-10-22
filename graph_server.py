#!/usr/bin/env python3

import socket
import threading
import os
import tempfile
import subprocess
import struct
import signal
import sys

flagChiusura = threading.Event()

def operazioniServer(socketClient):
    # connessione e comunicazione col client
    try:
        # lettura nome file, e numero nodi e archi
        header = socketClient.recv(8) # 8 bytes (4 n e 4 archiTot)
        n, archiTot = struct.unpack('ii', header) # ricevimento dell'header

        if n<=0 or archiTot<=0:
            print("Numero invalido di nodi o archi")
            return

        fileNameDim = struct.unpack('I', socketClient.recv(4))[0]
        fileName = socketClient.recv(fileNameDim).decode()

        # creazione di un file temporaneo per memorizzare il grafo
        tmpDir = tempfile.mkdtemp()
        tmpFilePath = os.path.join(tmpDir, fileName)

        # inizia a scrivere interi nel file
        archiValidi=0
        with open(tmpFilePath, 'w') as tmpFile:
            tmpFile.write("% File temporaneo\n")
            tmpFile.write(f"{n} {n} 0\n")

            # lettura degli archi
            archiRicevuti=0
            while archiRicevuti<archiTot:
                nodiArco=socketClient.recv(8) # 8 bytes (un arco è tra 2 nodi)
                if not nodiArco:
                    break

                # ottenimento dei dati sugli archi
                nu, ne = struct.unpack('ii', nodiArco)
                if 1<=nu<=n and 1<=ne<=n:
                    tmpFile.write(f"{nu} {ne}\n")
                    archiValidi+=1
                archiRicevuti+=1

        # aggiornamento degli archi validi nel file temporaneo
        if archiValidi>0:
            with open(tmpFilePath, 'r+') as tmpFile:
                linee = tmpFile.readlines() # lettura delle linee
                for i, linea in enumerate(linee):
                    if not linea.startswith('%'):
                        linee[i] = f"{n} {n} {archiValidi}\n" # aggiornamento del numero di archi validi
                        break
                # aggiorna il file (riscrivendo il contenuto)
                tmpFile.seek(0)
                tmpFile.writelines(linee)
                tmpFile.truncate() # rimuove righe in eccesso

        # stampa file temporaneo
        # with open(tmpFilePath, 'r') as tmpFile:
            #tmpFileContenuto = tmpFile.read()
        # print(f"Contenuto file temporaneo:\n{tmpFileContenuto}")

        # esecuzione pagerank
        exitCode=1
        if archiValidi>0:
            risultato = subprocess.run(
                ["./pagerank", tmpFilePath],
                capture_output=True,
                text=True
            )
            exitCode=risultato.returncode

            # stampa output 
            if exitCode==0:
                output = risultato.stdout
                linee = output.split('\n')
                output = '\n'.join([f"{fileName} {linea}" for linea in linee])
                socketClient.sendall(f"Exit code: {exitCode}\n{output}".encode())
            # stampa errori
            else:
                socketClient.sendall(f"Exit code: {exitCode}\n{fileName} Error: {risultato.stderr}".encode())
        else:
            socketClient.sendall(f"Exit code: {exitCode}\n{fileName} Nessun arco valido".encode())

    except Exception as errore:
        print(f"Error: {errore}")
    finally:
        socketClient.close()

def segnale(sig, frame):
    flagChiusura.set()

def main():
    global flagChiusura

    signal.signal(signal.SIGINT, segnale)

    # configurazione server
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    HOST = "127.0.0.1"
    PORT = 56646
    server.bind((HOST, PORT))
    server.listen(5)
    print(f"Server connesso su {HOST}:{PORT}")

    # gestione connessioni
    while not flagChiusura.is_set():
        try:
            server.settimeout(1.0) # timeout per evitare che si blocchi con accept()
            socketClient2, addr = server.accept() # accetta connessione dal client
            threadClient = threading.Thread(target=operazioniServer, args=(socketClient2,)) # thread per gestire il client
            threadClient.start()
        except socket.timeout:
            continue
        except Exception as errore:
            print(f"Error: {errore}")
            break

    # chiusura server
    server.close()
    print("Bye dal server") # premere INVIO successivamente

if __name__ == "__main__":
    main()

