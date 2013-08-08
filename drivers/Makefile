# Makefile for Raspberry Pi drivers

CC = gcc
CFLAGS = 

all: gpio.o test_gpio.x test_uart.x

gpio.o: gpio.c gpio.h
	$(CC) $(CFLAGS) -c gpio.c

test_gpio.x: test_gpio.o gpio.o
	$(CC) test_gpio.o gpio.o -o test_gpio.x

test_gpio.o: test_gpio.c gpio.h
	$(CC) $(CFLAGS) -c test_gpio.c

test_uart.x: test_uart.o gpio.o
	$(CC) test_uart.o gpio.o -o test_uart.x

test_uart.o: test_uart.c
	$(CC) $(CFLAGS) -c test_uart.c

clean:
	rm -f *.o *.x

