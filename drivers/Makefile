# Makefile for Raspberry Pi drivers

CC = gcc
CFLAGS = -g

all: gpio.o test_gpio.x

gpio.o: gpio.c gpio.h
	$(CC) $(CFLAGS) -c gpio.c

test_gpio.x: test_gpio.o gpio.o
	$(CC) test_gpio.o gpio.o -o test_gpio.x

test_gpio.o: test_gpio.c
	$(CC) $(CFLAGS) -c test_gpio.c


clean:
	rm *.o *.x

