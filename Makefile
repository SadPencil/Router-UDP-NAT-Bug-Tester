CC = gcc
CFLAGS = -g -Wall -O2

objects = client.o dns_c.h constructor.o c_utils.o parser.o

all: build

build: shell_lib.a
	$(CC) $(CFLAGS) -o client client.c dns_client.a

shell_lib.a: $(objects)
	ar -r dns_client.a $(objects)


test: build
	./client

clean:
	rm -f client
	rm -f *.o
	rm -f dns_client.a
