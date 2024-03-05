CFLAGS = -O3 -Wall

.PHONY: all clean

all: main

main: main.o murmur3.o hashtable.o
	$(CC) $^ -o $@

clean:
	rm -rf main *.o