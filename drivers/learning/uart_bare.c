/**
	Philip Romano
	Test for UART
*/

#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

static void ignoreInput();

// Size of page
#define BCM2835_PAGE_SIZE  (4*1024)
// Size of memory block
#define BCM2835_BLOCK_SIZE (4*1024)

#define UART_BASE          0x20201000

#define UART_OFFSET_DR     0x00000000
#define UART_OFFSET_RSRECR 0x00000004
#define UART_OFFSET_FR     0x00000018
#define UART_OFFSET_IBRD   0x00000024
#define UART_OFFSET_FBRD   0x00000028
#define UART_OFFSET_LCRH   0x0000002C
#define UART_OFFSET_CR     0x00000030
#define UART_OFFSET_IFLS   0x00000034
#define UART_OFFSET_IMSC   0x00000038
#define UART_OFFSET_RIS    0x0000003C
#define UART_OFFSET_MIS    0x00000040
#define UART_OFFSET_ICR    0x00000044

int main(int argc, char **argv) {
	int memfd;
	volatile uint8_t *uart_base = 0;

	memfd = open("/dev/mem", O_RDWR | O_SYNC);
	if (memfd == -1) {
		fprintf(stderr, "Couldn't open /dev/mem\n");
		return 1;
	}
	printf("Opened /dev/mem\n");

	uart_base = mmap(NULL, BCM2835_BLOCK_SIZE, PROT_READ | PROT_WRITE,
			MAP_SHARED, memfd, UART_BASE);
	if (uart_base == MAP_FAILED) {
		close(memfd);
		fprintf(stderr, "Couldn't create mapping to UART_BASE\n");
		return 1;
	}
	printf("Created mapping to UART_BASE\n");

	// Don't need file descriptor anymore
	close(memfd);
	printf("Closed file descriptor to memory\n");

	// First make sure UART is disabled
	uint32_t *cr = (uint32_t *)(uart_base + UART_OFFSET_CR);
	*cr = 0x0000;

	// Baud rate
	// UARTCLK = 7.3728 MHz = 7372800 Hz (init_uart_clock, /boot/settings.txt)
	// (Implicit divisor = 16)
	// Divisor   = 48.0   8.0
	// Baud rate = 9600   57600
	uint32_t *ibaud = (uint32_t *)(uart_base + UART_OFFSET_IBRD);
	uint32_t *fbaud = (uint32_t *)(uart_base + UART_OFFSET_FBRD);

	uint32_t ibaudval = *ibaud;
	uint32_t fbaudval = *fbaud;
	printf("UART baud divisor value: 0x%04X.%04X\n", ibaudval, fbaudval);

	*ibaud = 0x00000008; // 57600 baud
	*fbaud = 0x00000000;
	printf("UART baud rate set\n");

	ibaudval = *ibaud;
	fbaudval = *fbaud;
	printf("UART baud divisor value: 0x%04X.%04X\n", ibaudval, fbaudval);

	// UART settings
#define BRK  0x01  // Send break. 0:high, 1:low when not transmitting
#define PEN  0x02  // Parity enable
#define EPS  0x04  // Even parity select
#define STP2 0x08  // Two stop bits
#define FEN  0x10  // FIFO enable
#define WL5  0x00  // Word length 5-bits
#define WL6  0x20
#define WL7  0x40
#define WL8  0x60  // Word length 8-bits
#define SPS  0x80  // Stick parity select
	uint32_t *lcrh = (uint32_t *)(uart_base + UART_OFFSET_LCRH);
	uint32_t lcrhval = FEN | PEN | EPS | WL8;
	*lcrh = lcrhval;

	printf("UART line control settings set\n");

	uint32_t *imsc = (uint32_t *)(uart_base + UART_OFFSET_IMSC);
	uint32_t *mis = (uint32_t *)(uart_base + UART_OFFSET_MIS);
	*imsc = 0x000007F3; //0x000007F3; 0x000007B0

	// Enable UART
	*cr =  0x0000; // First disable
	*cr =  0x0300; // Transmit enable (bit 8) and Receive enable (bit 9)
	*cr |= 0x0001; // UART enable

	printf("Enabled UART\n");

	// Transmit loop.
	// Read from stdin and send to UART transmit
	uint32_t *dr = (uint32_t *)(uart_base + UART_OFFSET_DR);
	uint32_t *fr = (uint32_t *)(uart_base + UART_OFFSET_FR);
#define UART_FR_TXFF 0x0020 // Tx FIFO Full
#define UART_FR_RXFE 0x0010 // Rx FIFO Empty

	int done = 0;
	char buffer[50] = { 0 };
	while (!done) {
		fgets(buffer, 50, stdin);
		int length = strlen(buffer);
		if (buffer[length - 1] == '\n')
			buffer[length - 1] = '\0';

		if (strcmp(buffer, "q") == 0)
			done = 1;
		else {
			uint32_t writeout;
			uint32_t frval;
			char *c = buffer;
			while (*c != '\0') {
				writeout = (uint32_t)*c;
				while ((frval = (uint32_t)*fr) & UART_FR_TXFF)
					printf("Transmit FIFO Full\n");
				*dr = writeout;
				printf("Wrote 0x%08X to DR : FR = 0x%08X\n", writeout, frval);
				++c;
			}
			printf("\n");
		}
	}

	// Read whenever RX FIFO is not empty, until a ` is read
	uint32_t frval = 0;
	printf("Busy-waiting for RXFIFO to be not empty...\n");
	char c;
	done = 0;
	while (!done) {
		while (((frval = (uint32_t)*fr) & UART_FR_RXFE))
			;
		c = (char)(*dr & 0x000000FF);
		printf("%c", c);
		fflush(stdout);
		if (c == '`')
			done = 1;
	}
	printf("\n\n");

	printf("Dumping registers...\n\n");
	int i;
	for (i = 0; i <= 17; ++i) {
		uint32_t *reg = (uint32_t *)(uart_base + i * 4); // UART_OFFSET_FR);
		uint32_t regval = *reg;
		printf("Offset +0x%04X: 0x%08X\n", i * 4, regval);
	}

	// Disable UART before exiting
	*cr = 0x0000;
	printf("Disabled UART\n");

	// Close mapping
	if (munmap((uint8_t *)uart_base, BCM2835_PAGE_SIZE) < 0) {
		fprintf(stderr, "Failed to unmap UART_BASE. Continuing...\n");
	}
	printf("Closed mapping to UART_BASE\n");

	printf("Done!\n");
	return 0;
}

void ignoreInput() {
	char c;
	while ((c = getchar()) != '\n' && c != EOF);
}

