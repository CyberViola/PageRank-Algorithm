# definition of compiler and compilation flags
CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread

# executable name
EXEC=pagerank

# source files
SRCS=pagerank.c xerrori.c

# header files
HEADERS=structure.h xerrori.h

# object files
OBJS=$(SRCS:.c=.o)

# first target: executable creation
all: $(EXEC)

# executable creation rule
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# object files from source files creation rules
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

# object files and executable cleaning rule
clean:
	rm -f *.o $(EXEC)
	
