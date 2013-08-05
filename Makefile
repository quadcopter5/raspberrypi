CC = gcc
CFLAGS = -g
LDFLAGS = -lpthread -lrt -lxbee

all: learnlibxbee.x

learnlibxbee.x: learnlibxbee.c
	$(CC) $(CFLAGS) $(LDFLAGS) learnlibxbee.c -o learnlibxbee.x

