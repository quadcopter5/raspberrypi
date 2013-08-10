/**
	RaspberryPi GPIO Test
*/

#include <stdio.h>
#include <stdint.h>

#include <fcntl.h>
#include <sys/mman.h>

// Size of page block
#define BCM2835_PAGE_SIZE  (4*1024)
// Size of memory block
#define BCM2835_BLOCK_SIZE (4*1024)

#define GPIO_BASE          0x20200000

#define GPIO_OFFSET_FSEL0  0x00000000
#define GPIO_OFFSET_FSEL1  0x00000004
#define GPIO_OFFSET_FSEL2  0x00000008
#define GPIO_OFFSET_FSEL3  0x0000000C
#define GPIO_OFFSET_FSEL4  0x00000010
#define GPIO_OFFSET_FSEL5  0x00000014

#define GPIO_OFFSET_SET0   0x0000001C
#define GPIO_OFFSET_SET1   0x00000020

#define GPIO_OFFSET_CLR0   0x00000028
#define GPIO_OFFSET_CLR1   0x0000002C

static volatile uint8_t *gpio_base;

int main(int argc, char **argv) {
	// Setup BCM pin 17 as output
	int memfd = -1;
	memfd = open("/dev/mem", O_RDWR | O_SYNC);
	if (memfd < 0) {
		printf("Failed to open /dev/mem\n");
		printf("Make sure you have root privileges\n");
		return 1;
	}

	gpio_base = mmap(NULL, BCM2835_BLOCK_SIZE, PROT_READ | PROT_WRITE,
			MAP_SHARED, memfd, GPIO_BASE);
	if (gpio_base == MAP_FAILED) {
		printf("Failed to create memory mapping for GPIO_BASE\n");
		return 1;
	}

	uint32_t *fsel = (uint32_t *)(gpio_base + GPIO_OFFSET_FSEL1);
	*fsel = 00010000000; // bits 21-23 = 001 (output)

	// Set BCM pin 17 (high)
	uint32_t *set = (uint32_t *)(gpio_base + GPIO_OFFSET_SET0);
	*set = 0x1 << 17;

	char ignore = getchar();

	// Clear BCM pin 17 (low)
	uint32_t *clr = (uint32_t *)(gpio_base + GPIO_OFFSET_CLR0);
	*clr = 0x1 << 17;

	if (munmap(gpio_base, BCM2835_BLOCK_SIZE) < 0) {
		printf("Failed to unmap GPIO_BASE\n");
		return 1;
	}

	printf("Done!\n");
	return 0;
}

