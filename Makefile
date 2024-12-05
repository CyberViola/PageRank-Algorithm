# definizione del compilatore e dei flag di compilazione
CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread 
LDLIBS=-lm -lrt -pthread

# nome dell'eseguibile da creare
EXEC=pagerank

# file sorgente
SRCS=pagerank.c

# file oggetto
OBJS=$(SRCS:.c=.o)

# primo target: creazione eseguibile
all: $(EXEC)

# regola per creare l'eseguibile
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# regola per creare file oggetto da file sorgente
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# regola per pulire file oggetto e eseguibile
clean:
	rm -f *.o $(EXEC)
	
