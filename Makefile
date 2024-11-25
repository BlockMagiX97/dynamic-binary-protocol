CFLAGS := -g -O0

all: server.elf client.elf

server.elf: server.c protocol.h protocol.c
	gcc $(CFLAGS) -o bin/server.elf server.c protocol.c

client.elf: client.c protocol.h protocol.c
	gcc $(CFLAGS) -o bin/client.elf client.c protocol.c
