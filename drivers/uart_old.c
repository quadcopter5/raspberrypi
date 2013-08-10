/**
	Philip Romano
	UART Interface for Raspberry Pi (Broadcom 2835)
*/

#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

#include "uart.h"

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

// Line Control settings (LCRH bits)
#define LCRH_BRK  0x01  // Send break. 0:high, 1:low when not transmitting
#define LCRH_PEN  0x02  // Parity enable
#define LCRH_EPS  0x04  // Even parity select
#define LCRH_STP2 0x08  // Two stop bits
#define LCRH_FEN  0x10  // FIFO enable
#define LCRH_WL5  0x00  // Word length 5-bits
#define LCRH_WL6  0x20  // 6-bits
#define LCRH_WL7  0x40  // 7-bits
#define LCRH_WL8  0x60  // 8-bits
#define LCRH_SPS  0x80  // Stick parity select

// Flag Register bits
#define FR_TXFF 0x0020 // Tx FIFO Full
#define FR_RXFE 0x0010 // Rx FIFO Empty

// Pointer to the top of the *mapped memory* region of peripheral addresses
static volatile uint8_t *uart_base = 0;

// Offsets from uart_base to specific addresses
static volatile uint32_t *uart_dr   = 0; // Data Register
static volatile uint32_t *uart_cr   = 0; // Control Register
static volatile uint32_t *uart_fr   = 0; // Flag Register
static volatile uint32_t *uart_ibrd = 0; // Integer Baud Rate Divisor
static volatile uint32_t *uart_fbrd = 0; // Fractional Baud Rate Divisor
static volatile uint32_t *uart_lcrh = 0; // Line Control Register

// Internal error handling data
#define UART_ERRSIZE 128
static int error = 0;
static char error_str[UART_ERRSIZE];

static void generateError(const char *str) {
	error = 1;
	strncpy(error_str, str, UART_ERRSIZE);
}

int uart_init(int baudrate, UARTParity parity) {
	// Check validity of arguments
	if ((7372800 % (16 * baudrate)) != 0) {
		generateError("The provided baudrate is not supported");
		return 0;
	}

	if (parity > 2) {
		generateError("Unknown parity type provided");
		return 0;
	}

	// Get file descriptor to memory so we can create a memory mapping
	int memfd = open("/dev/mem", O_RDWR | O_SYNC);
	if (memfd == -1) {
		generateError("Couldn't open /dev/mem");
		return 1;
	}

#ifdef _DEBUG
	printf("Opened /dev/mem\n");
#endif

	// Get memory mapping so we have permission from kernel to write
	// to peripheral memory addresses
	uart_base = mmap(NULL, BCM2835_BLOCK_SIZE, PROT_READ | PROT_WRITE,
			MAP_SHARED, memfd, UART_BASE);
	if (uart_base == MAP_FAILED) {
		close(memfd);
		generateError("Couldn't create mapping to UART_BASE");
		return 0;
	}

#ifdef _DEBUG
	printf("Created mapping to UART_BASE\n");
#endif

	// Don't need file descriptor anymore
	close(memfd);

#ifdef _DEBUG
	printf("Closed file descriptor to memory\n");
#endif

	// Set pointers to memory mapped registers
	uart_dr   = (uint32_t *)(uart_base + UART_OFFSET_DR);
	uart_cr   = (uint32_t *)(uart_base + UART_OFFSET_CR);
	uart_fr   = (uint32_t *)(uart_base + UART_OFFSET_FR);
	uart_ibrd = (uint32_t *)(uart_base + UART_OFFSET_IBRD);
	uart_fbrd = (uint32_t *)(uart_base + UART_OFFSET_FBRD);
	uart_lcrh = (uint32_t *)(uart_base + UART_OFFSET_LCRH);

	// Make sure UART is disabled before changing settings!
	*uart_cr = 0x0000;

	// Baud rate
	// 
	// UARTCLK = 7.3728 MHz = 7372800 Hz (init_uart_clock, /boot/settings.txt)
	// (Implicit divisor = 16)
	//
	// Divisor = UARTCLK / (16 * baudrate)  (from ARM UART PL011)
	//
	// Divisor   = 48.0   8.0
	// Baud rate = 9600   57600
	uint16_t divisor = 7372800 / (16 * baudrate);
	*uart_ibrd = 0x00000030; // 9600 baud
	*uart_fbrd = 0x00000000;

#ifdef _DEBUG
	printf("UART baud rate set. Intended baud rate: %d\n", baudrate);
	uint32_t ibaudval = *uart_ibrd;
	uint32_t fbaudval = *uart_fbrd;
	printf("UART baud divisor value: 0x%04X.%04X\n", ibaudval, fbaudval);
#endif

	// Line Control register
	uint32_t lcrhval = LCRH_FEN | LCRH_WL8;
	if (parity != UART_PARDISABLE) {
		lcrhval |= LCRH_PEN;
		if (parity == UART_PAREVEN)
			lcrhval |= LCRH_EPS;
	}
	*uart_lcrh = lcrhval;

#ifdef _DEBUG
	printf("UART line control settings set\n");
#endif

	// Enable UART. Already disabled above; safe to modify CR
	*uart_cr =  0x0300; // Transmit enable (bit 8) and Receive enable (bit 9)
	*uart_cr |= 0x0001; // UART enable

#ifdef _DEBUG
	printf("Enabled UART\n");
#endif
}

int uart_deinit() {
	// Disable UART before exiting
	*uart_cr = 0x0000;

#ifdef _DEBUG
	printf("Disabled UART\n");
#endif

	// Close mapping
	if (munmap((uint8_t *)uart_base, BCM2835_PAGE_SIZE) < 0)
		fprintf(stderr, "Failed to unmap UART_BASE. Continuing...\n");

#ifdef _DEBUG
	printf("Closed mapping to UART_BASE\n");
#endif

	return 1;
}

int uart_write(const void *buffer, size_t len) {
	uint32_t writeout;
	uint32_t frval;
	char *c = (char *)buffer;
	int i;

	for (i = 0; i < len; ++i) {
		writeout = (uint32_t)*c;

		// Bad busy-wait...
		// Hopefully implementing multi-threading will be able help
		while ((frval = *uart_fr) & FR_TXFF)
#ifdef _DEBUG
			printf("Transmit FIFO Full\n");
#else
			;
#endif

		*uart_dr = writeout;

#ifdef _DEBUG
		printf("Wrote 0x%08X to DR\n", writeout);
#endif
		++c;
	}
	
	return 1;
}

int uart_read(void *buffer, size_t len) {
	uint32_t frval = 0;
	char *cbuffer = (char *)buffer;
	char c = 0;
	int i = 0;

	while (i < len && !((frval = *uart_fr) & FR_RXFE)) {
		// AWFUL busy-wait...
		// Going to implement multi-threads, and have an intermediate
		// buffer
//		while ((frval = *uart_fr) & FR_RXFE)
//			;

		c = (char)(*uart_dr & 0x000000FF);
		*cbuffer = c;

		++i;
		++cbuffer;
	}

	return 1;
}

char *uart_getLastError() {
	if (error) {
		error = 0;
		return error_str;
	} else
		return "No error";
}

