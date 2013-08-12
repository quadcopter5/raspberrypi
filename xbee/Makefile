CC = gcc
CFLAGS = -g
LDFLAGS = 

all: usbxbee.x twoway.x

usbxbee.x: usbxbee.c
	$(CC) $(CFLAGS) $(LDFLAGS) usbxbee.c -o usbxbee.x

twoway.x: twoway.c
	$(CC) $(CFLAGS) $(LDFLAGS) twoway.c -o twoway.x

clean:
	rm -f *.o *.x

