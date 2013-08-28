/**
	Philip Romano
	Learning I2C interface
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>

#define PI 3.1415926535

void quit(int code);

struct termios in_oldattr, in_newattr;

int main(int argc, char **argv) {
	// Set up console for unbuffered input

	// Get current console termios attributes (so we can restore it later)
	tcgetattr(STDIN_FILENO, &in_oldattr);
	memcpy(&in_newattr, &in_oldattr, sizeof(struct termios));

	// Set console attributes
	in_newattr.c_lflag = 0;
	in_newattr.c_cc[VMIN] = 0;
	in_newattr.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSANOW, &in_newattr);

	// Open I2C bus
	int i2cfd = open("/dev/i2c-1", O_RDWR);
	if (i2cfd == -1) {
		fprintf(stderr, "Could not open /dev/i2c-1\n");
		quit(-1);
	}
	printf("Opened /dev/i2c-1\n");

	/*
		ADXL345
		Accelerometer
		Slave Address: 0x53
	*/
	printf("\n -- Accelerometer -- \n\n");
	uint8_t slaveaddr = 0x53;

	if (ioctl(i2cfd, I2C_SLAVE, slaveaddr) < 0) {
		fprintf(stderr, "Could not set I2C_SLAVE\n");
		quit(-1);
	}
	printf("Set I2C slave\n");

	int bufsize, bytes;
	char buffer[16];

	// Get out of sleep mode
	buffer[0] = 0x2D; // POWER_CTL register
	buffer[1] = 0b00001000; // Measure (bit 3), !Sleep (bit 4)

	struct i2c_rdwr_ioctl_data iodata;
	struct i2c_msg messages[10];

	messages[0].addr = slaveaddr;
	messages[0].flags = 0;
	messages[0].len = 2;
	messages[0].buf = &buffer[0];

	iodata.msgs = messages;
	iodata.nmsgs = 1;

	int iostatus;
	if ((iostatus = ioctl(i2cfd, I2C_RDWR, &iodata)) < 0) {
		fprintf(stderr, "ioctl() failed, return value %d\n", iostatus);
		quit(-1);
	}
	printf("Configured accelerometer\n");

	char input;
	while (read(STDIN_FILENO, &input, 1) < 1) {
		buffer[0] = 0x32; // DATAX0 register (beginning of all 6 data registers)

		int16_t values[3];

		messages[0].addr = slaveaddr;
		messages[0].flags = 0;
		messages[0].len = 1;
		messages[0].buf = &buffer[0];

		messages[1].addr = slaveaddr;
		messages[1].flags = I2C_M_RD;
		messages[1].len = 6;
		messages[1].buf = (uint8_t *)values;

		iodata.msgs = messages;
		iodata.nmsgs = 2;

		if ((iostatus = ioctl(i2cfd, I2C_RDWR, &iodata)) < 0) {
			fprintf(stderr, "ioctl() failed, return value %d\n", iostatus);
			quit(-1);
		}

		int i;
		for (i = 0; i < 3; ++i)
			printf("Value %d = %+0.5d | ", i, values[i]);
		printf("\n");

		usleep(50000);
	}

	// Put to sleep
	buffer[0] = 0x2D; // POWER_CTL register
	buffer[1] = 0b00001100; // Measure (bit 3), !Sleep (bit 4)

	messages[0].addr = slaveaddr;
	messages[0].flags = 0;
	messages[0].len = 2;
	messages[0].buf = &buffer[0];

	iodata.nmsgs = 1;

	if ((iostatus = ioctl(i2cfd, I2C_RDWR, &iodata)) < 0) {
		fprintf(stderr, "ioctl() failed, return value %d\n", iostatus);
		quit(-1);
	}
	printf("Put accelerometer to sleep\n");

	while (read(STDIN_FILENO, &input, 1) < 1)
		usleep(1000);


	/*
		HMC5883L
		Magnetometer
		Slave Address: 0x1E
	*/
	printf("\n -- Magnetometer -- \n\n");
	slaveaddr = 0x1E;

	if (ioctl(i2cfd, I2C_SLAVE, slaveaddr) < 0) {
		fprintf(stderr, "Could not set I2C_SLAVE\n");
		quit(-1);
	}
	printf("Set I2C slave\n");

	bufsize = 4;
	buffer[0] = 0x00; // Control Register A
	buffer[1] = 0b01110000; // 8 samples averaged (bits 5 - 6)
	                        // 15 Hz data output rate (bits 2 - 4)
	buffer[2] = 0x02; // Mode Register
	buffer[3] = 0x00; //   Continuous-Measurement Mode
	if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize) {
		fprintf(stderr, "write() failed. Return value = %d\n", bytes);
		quit(-1);
	}

	int done = 0;
	while (!done) {

		usleep(50000);

		// Check if status register says data is ready
		char status = 0;
		do {
			bufsize = 1;
			buffer[0] = 0x09; // Status Register
			if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize) {
				fprintf(stderr, "write() failed. Return value = %d\n", bytes);
				quit(-1);
			}

			bufsize = 1;
			if ((bytes = read(i2cfd, &status, bufsize)) < bufsize) {
				int errread = errno;
				fprintf(stderr, "read() failed. Return value = %d\n", bytes);
				if (bytes == -1)
					printf("errno = %d : %s\n", errread, strerror(errread));
				quit(-1);
			}

			// 0x01 of Status Register is RDY bit
			if (!(status & 0x01))
				usleep(100);

		} while (!(status & 0x01));

		// Read data out
		bufsize = 1;
		buffer[0] = 0x03; // Data Out X MSB (DXRA)
		if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize) {
			fprintf(stderr, "write() failed. Return value = %d\n", bytes);
			quit(-1);
		}

		int16_t values[3];
		if ((bytes = read(i2cfd, values, 6)) < 6) {
			int errread = errno;
			fprintf(stderr, "read() failed. Return value = %d\n", bytes);
			if (bytes == -1)
				printf("errno = %d : %s\n", errread, strerror(errread));
			quit(-1);
		}

		double magnitude = sqrt(pow(values[0], 2.0) + pow(values[1], 2.0) + pow(values[2], 2.0));
		double angle = (acos(values[0] / magnitude) * 180.0) / PI;

		int i;
		for (i = 0; i < 3; ++i)
			printf("Value %d = %+0.5d | ", i, values[i]);
		printf("Magnitude = %f | Angle = %f deg", magnitude, angle);
		printf("\n");

		char input;
		if (read(STDIN_FILENO, &input, 1) == 1)
			done = 1;
	}

	bufsize = 2;
	buffer[0] = 0x02; // Mode Register
	buffer[1] = 0x02; //   Idle Mode
	if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize) {
		fprintf(stderr, "write() failed. Return value = %d\n", bytes);
		quit(-1);
	}
	printf("Put magnetometer into idle mode\n");

	while (read(STDIN_FILENO, &input, 1) < 1)
		usleep(1000);


	/*
		L3G4200D
		Gyroscope
		Slave Address: 0x69
	*/
	printf("\n -- Gyroscope -- \n\n");
	slaveaddr = 0x69; // 0x69 if SDO is connected to power, 0x68 otherwise

	if (ioctl(i2cfd, I2C_SLAVE, slaveaddr) < 0) {
		fprintf(stderr, "Could not set I2C_SLAVE\n");
		quit(-1);
	}
	printf("Set I2C slave\n");

	// Set control registers
	bufsize = 6;
	buffer[0] = 0x20 | 0x80; // CTRL_REG1, auto-increment
	buffer[1] = 0b00001111; // ODR = 100Hz. Normal mode (bit 3). X,Y,Z enabled
	buffer[2] = 0b00000000;
	buffer[3] = 0b00000000;
	buffer[4] = 0b00010000; // Little_endian (bit 6), Full Scale set to 500 dps (bits 4-5)
	buffer[5] = 0b00000000; // FIFO disable (bit 6), Low-Pass filter 2 disabled (bit 1)
	if ((bytes = write(i2cfd, buffer, bufsize)) < bufsize) {
		fprintf(stderr, "write() failed. Return value = %d\n", bytes);
		quit(-1);
	}

	printf("Configured gyroscope\n");

	// Sample averaging
#define NUMSAMPLES 5
	int16_t values[NUMSAMPLES][3];
	int16_t average[3];
	int currentsample = 0;

	done = 0;
	while (!done) {

		usleep(5000);

		char status = 0;

		buffer[0] = 0x27; // STATUS_REG. No auto-increment (MSb is 0)

		messages[0].addr  = slaveaddr;
		messages[0].flags = 0;
		messages[0].len   = 1;
		messages[0].buf   = &buffer[0];

		messages[1].addr  = slaveaddr;
		messages[1].flags = I2C_M_RD;
		messages[1].len   = 1;
		messages[1].buf   = &status;

		iodata.nmsgs = 2;

		if ((iostatus = ioctl(i2cfd, I2C_RDWR, &iodata)) < 0) {
			fprintf(stderr, "ioctl() failed, return value %d\n", iostatus);
			quit(-1);
		}

		if (status & 0x08) {
			// Read data
			buffer[0] = 0x28 | 0x80; // OUT_X_L. Using auto-increment (MSb is 1)

			messages[0].addr  = slaveaddr;
			messages[0].flags = 0;
			messages[0].len   = 1;
			messages[0].buf   = &buffer[0];

			messages[1].addr  = slaveaddr;
			messages[1].flags = I2C_M_RD;
			messages[1].len   = 6;
			messages[1].buf   = (uint8_t *)&values[currentsample][0];

			iodata.nmsgs = 2;

			if ((iostatus = ioctl(i2cfd, I2C_RDWR, &iodata)) < 0) {
				fprintf(stderr, "ioctl() failed, return value %d\n", iostatus);
				quit(-1);
			}

			++currentsample;

			if (currentsample >= NUMSAMPLES) {
				// Average the gathered samples
				int i, s;
				for (i = 0; i < NUMSAMPLES; ++i) {
					for (s = 0; s < 3; ++s)
						average[s] += values[i][s];
				}
				for (s = 0; s < 3; ++s)
					average[s] = average[s] / NUMSAMPLES;

				// Print results
				printf("Status = 0x%02X | ", status);
				for (i = 0; i < 3; ++i)
					printf("Value %d = %+06d | ", i, average[i]);
				printf("\n");

				// Reset back to beginning of sample buffer
				currentsample = 0;
			}
		}

		char input;
		if (read(STDIN_FILENO, &input, 1) == 1)
			done = 1;
	}

	buffer[0] = 0x20; // CTRL_REG1
	buffer[1] = 0b00000000; // Power Down mode (bit 3)
	messages[0].addr = slaveaddr;
	messages[0].flags = 0;
	messages[0].len = 2;
	messages[0].buf = &buffer[0];
	iodata.nmsgs = 1;

	if ((iostatus = ioctl(i2cfd, I2C_RDWR, &iodata)) < 0) {
		fprintf(stderr, "ioctl() failed, return value %d\n", iostatus);
		quit(-1);
	}

	printf("Put gyroscope in power down mode\n");

	// Close up shop
	close(i2cfd);
	printf("\nClosed /dev/i2c-1\n");

	printf("Done!\n");
	quit(0);
}

void quit(int code) {
	// Reset to old console attributes
	tcsetattr(STDIN_FILENO, TCSANOW, &in_oldattr);
	exit(code);
}

