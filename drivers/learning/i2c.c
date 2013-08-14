/**
	Philip Romano
	Learning I2C interface
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>

static void ignoreInput();

int main(int argc, char **argv) {
	int i2cfd = open("/dev/i2c-1", O_RDWR);
	if (i2cfd == -1) {
		fprintf(stderr, "Could not open /dev/i2c-1\n");
		return -1;
	}
	printf("Opened /dev/i2c-1\n");

	if (ioctl(i2cfd, I2C_SLAVE, 0b01000000) < 0) {
		fprintf(stderr, "Could not set I2C_SLAVE\n");
		return -1;
	}
	printf("Set I2C slave\n");

	/*
	// Single-direction read
	uint16_t value;
	int bytes;
	if ((bytes = read(i2cfd, &value, 2)) < 2) {
		int errread = errno;
		fprintf(stderr, "read() failed. Return value = %d\n", bytes);
		if (bytes == -1)
			printf("errno = %d : %s\n", errread, strerror(errread));
		return -1;
	}
	printf("read() succeeded\n");
	printf("  Value is : %d\n", value);
	*/

	int bufsize, bytes;
	char buffer[64];

	// Initialize PCA9685
	bufsize = 2;
	buffer[0] = 0b00000000;
	buffer[1] = 0b00110000; // Auto-Increment (bit 5) | Sleep (bit 4)

	if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize) {
		fprintf(stderr, "write() failed. Return value = %d\n", bytes);
		return -1;
	}

	if (usleep(1000) == -1) {
		fprintf(stderr, "usleep() failed\n");
		return -1;
	}

	bufsize = 4;
	buffer[0] = 0b11111110; // PRE_SCALE
	buffer[1] = 0x0079;     // (25000000Hz / (4096 * freq)) - 1
	                        // freq = 50Hz, prescale = 121 = 0x79
	buffer[2] = 0b00000000; // MODE1
	buffer[3] = 0b00100000; // Auto-Increment | Not Sleep
	write(i2cfd, buffer, bufsize);

	// Must be not sleeping for at least 500us before writing to Reset bit
	usleep(1000);

	bufsize = 2;
	buffer[0] = 0b00000000;
	buffer[1] = 0b10100000; // Reset | Auto-Increment
	write(i2cfd, buffer, bufsize);

	usleep(1000);

	printf("Initialied PWM\n");

	uint16_t offcount;
	printf("Enter OFF count: ");
	scanf("%d", &offcount);

	printf("Using value 0x%04X\n", offcount & 0xFFFF);

	// Set Channel 7
	bufsize = 5;
	buffer[0] = 0b00100010; // LED7_ON_L (base register address)
	buffer[1] = 0x00; // LED7_ON_L
	buffer[2] = 0x00; // LED7_ON_H
	buffer[3] = (char)(offcount & 0x00FF); // 0xFF; // LED7_OFF_L
	buffer[4] = (char)((offcount & 0x0F00) >> 8); // LED7_OFF_H

	if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize) {
		int err = errno;
		fprintf(stderr, "write() failed. Return value = %d\n", bytes);
		if (bytes == -1)
			printf("errno = %d : %s\n", err, strerror(err));
		return -1;
	}
	printf("Set Channel 7\n");

	/*
	// Simultaneous read/write

	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg messages[2];

	char msgbuf[2];

	while (1) {
		msgbuf[0] = 0b11111110;
		msgbuf[1] = 0b00000000;

		messages[0].addr  = 0b01000000;
		messages[0].flags = I2C_M_RD;
		messages[0].len   = 2;
		messages[0].buf   = &msgbuf[0];

	//	messages[1].addr  = 0b01000000;
	//	messages[1].flags = I2C_M_RD;
	//	messages[1].len   = 1;
	//	messages[1].buf   = &msgbuf[1];

		data.msgs  = messages;
		data.nmsgs = 2;

		if (ioctl(i2cfd, I2C_RDWR, &data) < 0) {
			fprintf(stderr, "ioctl(I2C_RDWR) failed\n");
			return -1;
		}
//		printf("ioctl(I2C_RDWR) succeeded\n");
//		printf("  Received value : %d\n", msgbuf[0]);
//		printf("  Sent value     : %d\n", msgbuf[1]);
	}
	*/

	close(i2cfd);
	printf("Closed /dev/i2c-1\n");

	printf("Done!\n");
	return 0;
}

void ignoreInput() {
	char c;
	while ((c = getchar()) != '\n' && c != EOF);
}

