/**
	Philip Romano
	Learning I2C interface
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>

static void ignoreInput();

int main(int argc, char **argv) {
	// Set up console for unbuffered input

	// Get current console termios attributes (so we can restore it later)
	struct termios in_oldattr, in_newattr;
	tcgetattr(STDIN_FILENO, &in_oldattr);
	memcpy(&in_newattr, &in_oldattr, sizeof(struct termios));

	// Set console attributes
	in_newattr.c_lflag = 0;
	in_newattr.c_cc[VMIN] = 1;
	in_newattr.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSANOW, &in_newattr);

	// Open file descriptor to I2C bus
	int i2cfd = open("/dev/i2c-1", O_RDWR);
	if (i2cfd == -1) {
		fprintf(stderr, "Could not open /dev/i2c-1\n");
		return -1;
	}
	printf("Opened /dev/i2c-1\n");

	// Set I2C slave address
	if (ioctl(i2cfd, I2C_SLAVE, 0b01000000) < 0) {
		fprintf(stderr, "Could not set I2C_SLAVE\n");
		return -1;
	}
	printf("Set I2C slave\n");

	int  bufsize, bytes;
	char buffer[64];
	int  channel = 0;

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

	// Get PWM channel from user
	printf("Enter PWM channel to control: ");
	fflush(stdout);
	
	memset(buffer, 0, 10);
	int pos = 0;
	char last = 0;
	while (pos < 9 && last != '\n') {
		read(STDIN_FILENO, &buffer[pos], 1);
		write(STDOUT_FILENO, &buffer[pos], 1);
		last = buffer[pos];
		++pos;
	}
	buffer[pos] = '\0';
	channel = atoi(buffer);

	if (channel < 0) {
		printf("Using channel 0\n");
		channel = 0;
	} else if (channel > 15) {
		printf("Using channel 15\n");
		channel = 15;
	}

	// Set initial PWM load (1.5 ms)
	float time_high = 1.5f; // Amount of time in HIGH voltage, in ms
	uint16_t offcount = (int)((time_high / 20.0f) * 4096.0f);

	bufsize = 5;
	buffer[0] = 0b00000110 + channel * 4; // LEDx_ON_L (base register address)
	buffer[1] = 0x00; // LEDx_ON_L
	buffer[2] = 0x00; // LEDx_ON_H
	buffer[3] = (char)(offcount & 0x00FF); // 0xFF; // LED7_OFF_L
	buffer[4] = (char)((offcount & 0x0F00) >> 8); // LED7_OFF_H

	if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize) {
		fprintf(stderr, "write() failed. Return value = %d\n", bytes);
	}
	printf("Set initial value on Channel %d (%.3f ms HIGH)\n",
			channel, time_high);

	printf("Initialized PWM\n");


	// Main loop
	printf("\nPress 'q' to quit\n\n");

	char c = 0;
	char buf[3] = { 0 };
	char arrowkey = 0;
	float delta = 0.02f;

	while (c != 'q') {

		// Check for input

		if (read(STDIN_FILENO, &c, 1) == 1) {
			// Got input

			// Figure out if it was an arrow key
			// Up    = 27 91 65
			// Down  = 27 91 66
			// Right = 27 91 67
			// Left  = 27 91 68

			if (c == 27 && buf[0] == 0)
				buf[0] = c;
			else if (c == 91 && buf[0] == 27 && buf[1] == 0)
				buf[1] = c;
			else if (buf[0] == 27 && buf[1] == 91) {
				arrowkey = c;
				memset(buf, 0, 3);
			}
			else
				memset(buf, 0, 3);
		} else
			// No input this loop
			memset(buf, 0, 3);

		if (arrowkey) {
			switch (arrowkey) {
				case 65:
					// Up arrow key
					time_high += delta;
					if (time_high > 2.6f)
						time_high = 2.6f;
					break;

				case 66:
					// Down arrow key
					time_high -= delta;
					if (time_high < 0.6f)
						time_high = 0.6f;
					break;

				case 67:
					// Right arrow key
					delta += 0.01f;
					if (delta > 2.0f)
						delta = 2.0f;
					printf("Delta set to %.3f\n", delta);
					break;

				case 68:
					// Left arrow key
					delta -= 0.01f;
					if (delta < 0.0f)
						delta = 0.0f;
					printf("Delta set to %.3f\n", delta);
					break;

				default:
					break;
			}
		}

		if (arrowkey == 65 || arrowkey == 66) {
			// Only write changes in load when actually changed
			// (UP or DN pressed)
			offcount = (int)((time_high / 20.0f) * 4096.0f);

			bufsize = 5;
			buffer[0] = 0b00000110 + channel * 4; // LEDx_ON_L (base register address)
			buffer[1] = 0x00; // LEDx_ON_L
			buffer[2] = 0x00; // LEDx_ON_H
			buffer[3] = (char)(offcount & 0x00FF); // 0xFF; // LED7_OFF_L
			buffer[4] = (char)((offcount & 0x0F00) >> 8); // LED7_OFF_H

			if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize)
				fprintf(stderr, "write() failed. Return value = %d\n", bytes);

			printf("HIGH time : (%.3f ms, %d counts)\n", time_high, offcount);
		}

		arrowkey = 0;
	}
	printf("\n");

	// Reset to old console attributes
	tcsetattr(STDIN_FILENO, TCSANOW, &in_oldattr);

	/*
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

