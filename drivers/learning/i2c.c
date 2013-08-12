/**
	Philip Romano
	Learning I2C interface
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>

static void ignoreInput();

int main(int argc, char **argv) {
	int i2cfd = open("/dev/i2c-0", O_RDWR);
	if (i2cfd == -1) {
		fprintf(stderr, "Could not open /dev/i2c-0\n");
		return -1;
	}
	printf("Opened /dev/i2c-0\n");

	if (ioctl(i2cfd, I2C_SLAVE, 0b00000001) < 0) {
		fprintf(stderr, "Could not set I2C_SLAVE\n");
		return -1;
	}
	printf("Set I2C slave\n");

	// Single-direction read
	uint16_t value;
	if (read(i2cfd, &value, 2) < 2) {
		fprintf(stderr, "read() failed\n");
		return -1;
	}
	printf("read() succeeded\n");
	printf("  Value is : %d\n", value);

	// Simultaneous read/write

	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg messages[2];
	uint16_t msgbuf[2];

	messages[0].addr  = 0b00000001;
	messages[0].flags = I2C_M_RD;
	messages[0].len   = 2;
	messages[0].buf   = (uint8_t *)&msgbuf[0];

	messages[1].addr  = 0b00000001;
	messages[1].flags = 0;
	messages[1].len   = 2;
	messages[1].buf   = (uint8_t *)&msgbuf[1];

	msgbuf[1] = 0x001C; // 28

	data.msgs  = messages;
	data.nmsgs = 2;

	if (ioctl(i2cfd, I2C_RDWR, &data) < 0) {
		fprintf(stderr, "ioctl(I2C_RDWR) failed\n");
		return -1;
	}
	printf("ioctl(I2C_RDWR) succeeded\n");
	printf("  Received value : %d\n", msgbuf[0]);
	printf("  Sent value     : %d\n", msgbuf[1]);

	close(i2cfd);
	printf("Closed /dev/i2c-0\n");

	printf("Done!\n");
	return 0;
}

void ignoreInput() {
	char c;
	while ((c = getchar()) != '\n' && c != EOF);
}

