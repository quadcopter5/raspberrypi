CC = gcc
CFLAGS = 

all: gpio_bare.x uart_bare.x twoway.x

gpio_bare.x: gpio_bare.c
	$(CC) $(CFLAGS) gpio_bare.c -o gpio_bare.x

uart_bare.x: uart_bare.c
	$(CC) $(CFLAGS) uart_bare.c -o uart_bare.x

twoway.x: twoway.c
	$(CC) $(CFLAGS) twoway.c -o twoway.x

clean:
	rm -f *.o *.x

