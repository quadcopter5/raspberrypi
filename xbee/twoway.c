/*
	8/12/2013
	Interfacing with XBee through Linux USB. Easy as pie!

	Extremely simple two-way communication. If there is something to received, then
	output it to standard output. Otherwise, get standard input and transmit it.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char **argv) {
	int     usbdev = 0,
	        baudarg = 9600,
	        parity = 0;
	speed_t baudrate = B9600;
	char    devname[64];

	// For the purposes of this program:
	// 0 = no parity, 1 = odd, 2 (or anything else) = even parity

	if (argc >= 2)
		usbdev = atoi(argv[1]);
	if (argc >= 3) {
		baudarg = atoi(argv[2]);
		switch (baudarg) {
			case 1200:
				baudrate = B1200;
				break;
			case 1800:
				baudrate = B1800;
				break;
			case 2400:
				baudrate = B2400;
				break;
			case 4800:
				baudrate = B4800;
				break;
			case 9600:
				baudrate = B9600;
				break;
			case 19200:
				baudrate = B19200;
				break;
			case 38400:
				baudrate = B38400;
				break;
			case 57600:
				baudrate = B57600;
				break;
			case 115200:
				baudrate = B115200;
				break;
			case 230400:
				baudrate = B230400;
				break;

			default:
				printf("WARNING: unsupported baudrate, using 9600\n");
				baudarg = 9600;
				baudrate = B9600;
				break;
		}
	}
	if (argc >= 4)
		parity = atoi(argv[3]);

	snprintf(devname, 64, "/dev/ttyUSB%d", usbdev);

	printf("Using %s\n", devname);
	printf("Using baud rate %d\n", baudarg);
	if (parity == 0)
		printf("No parity\n");
	else if (parity == 1)
		printf("Odd parity\n");
	else
		printf("Even parity\n");

	// Open USB interface
	int fd = open(devname, O_RDWR | O_ASYNC);
	if (fd == -1) {
		fprintf(stderr, "Could not open %s\n", devname);
		return 1;
	}
	printf("Opened %s\n", devname);

	// Set terminal properties for /dev/ttyUSBx
	struct termios tprops;
	tcgetattr(fd, &tprops);

	tprops.c_iflag = 0;
	if (parity)
		tprops.c_iflag |= INPCK;

	tprops.c_oflag = 0;

	tprops.c_cflag = CS8 | CREAD;
	if (parity) {
		tprops.c_cflag |= PARENB;
		if (parity == 1)
			tprops.c_cflag |= PARODD;
	}

	tprops.c_lflag = 0;

	// Disable control characaters
	int cc;
	for (cc = 0; cc < NCCS; ++cc)
		tprops.c_cc[cc] = _POSIX_VDISABLE;

	tprops.c_cc[VMIN]  = 0; // No minimum number of characters for reads
	tprops.c_cc[VTIME] = 0; // No timeout (0 deciseconds)

	// Baud rate
	cfsetospeed(&tprops, baudrate);
	cfsetispeed(&tprops, baudrate);

	tcsetattr(fd, TCSAFLUSH, &tprops);

	tcflow(fd, TCOON | TCION);
	tcflush(fd, TCIOFLUSH);

	char inbuffer[50],
	     outbuffer[50];
	int  bytes = 0;
	int  done = 0;
	while (!done) {
		// Receive
		printf("Checking for received input\n");
		while ((bytes = read(fd, inbuffer, 50)) > 0 && !done) {
			char *c;
			while ((c = memchr(inbuffer, '\r', bytes)) != NULL)
				*c = '\n';
			fwrite(inbuffer, 1, bytes, stdout);
			if (memchr(inbuffer, '`', bytes) != NULL)
				done = 1;
		}
		printf("\n");

		if (!done) {
			// Transmit
			printf("Send message: ");
			fgets(outbuffer, 50, stdin);
			if (strchr(outbuffer, '`') != NULL)
				done = 1;

			if (outbuffer[strlen(outbuffer) - 1] == '\n')
				outbuffer[strlen(outbuffer) - 1] = '\r';
			if (strlen(outbuffer) > 0 && strcmp(outbuffer, "\r") != 0) {
				write(fd, outbuffer, strlen(outbuffer));
				printf("Wrote data\n");
			}
		}
	}
	printf("\n");

	close(fd);
	printf("Closed %s\n", devname);

	return 0;
}

