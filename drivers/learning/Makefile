CC = gcc
CFLAGS = -g

all: gpio_bare.x uart_bare.x twoway.x i2c.x

gpio_bare.x: gpio_bare.c
	$(CC) $(CFLAGS) gpio_bare.c -o gpio_bare.x

uart_bare.x: uart_bare.c
	$(CC) $(CFLAGS) uart_bare.c -o uart_bare.x

twoway.x: twoway.c
	$(CC) $(CFLAGS) twoway.c -o twoway.x

i2c.x: i2c.c
	$(CC) $(CFLAGS) i2c.c -o i2c.x

clean:
	rm -f *.o *.x

