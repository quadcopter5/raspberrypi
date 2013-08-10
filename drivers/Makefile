# Makefile for Raspberry Pi drivers

CC = gcc
CFLAGS = -g -D_DEBUG

all: gpio.o uart.o test_gpio.x test_uart.x


gpio.o: gpio.c gpio.h
	$(CC) $(CFLAGS) -c gpio.c

uart.o: uart.c uart.h
	$(CC) $(CFLAGS) -c uart.c

test_gpio.x: test_gpio.o gpio.o
	$(CC) test_gpio.o gpio.o -o test_gpio.x

test_gpio.o: test_gpio.c gpio.h
	$(CC) $(CFLAGS) -c test_gpio.c

test_uart.x: test_uart.o uart.o
	$(CC) test_uart.o uart.o -o test_uart.x

test_uart.o: test_uart.c uart.h
	$(CC) $(CFLAGS) -c test_uart.c


clean:
	rm -f *.o *.x

