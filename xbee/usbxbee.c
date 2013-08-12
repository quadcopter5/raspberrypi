/*
	8/8/2013
	Interfacing with XBee through Linux USB. Easy as pie!
*/

#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <termios.h>

int main(int argc, char **argv) {
	// Open USB interface
	int fd = open("/dev/ttyUSB0", O_RDWR | O_ASYNC);
	if (fd == -1) {
		fprintf(stderr, "Could not open /dev/ttyUSB0\n");
		return 1;
	}
	printf("Opened /dev/ttyUSB0\n");

	// Set terminal properties for /dev/ttyUSBx
	struct termios tprops;
	tcgetattr(fd, &tprops);

	tprops.c_iflag = INPCK;
	tprops.c_cflag = CS8 | CREAD | PARENB;
	tprops.c_lflag = 0;

	// Baud rate
	cfsetospeed(&tprops, B57600);
	cfsetispeed(&tprops, B57600);

	tcsetattr(fd, TCSAFLUSH, &tprops);

	// Transmit
	char *str = "Hello!";
	write(fd, str, strlen(str));
	printf("Wrote \"%s\" to /dev/ttyUSB0\n", str);

	// Receive
	printf("Busy-waiting for input\n");
	char inbuffer[50];
	int bytes = 0;
	while (strncmp(inbuffer, "`", 1) != 0) {
	   bytes = read(fd, inbuffer, 50);
	   if (bytes > 0) {
		   printf("(%02d bytes) | ", bytes);
		   fwrite(inbuffer, 1, bytes, stdout);
		   printf("\n");
	   }
	}
	printf("\n");

	close(fd);
	printf("Closed /dev/ttyUSB0\n");

	return 0;
}

