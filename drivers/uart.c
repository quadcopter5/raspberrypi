/**
	Philip Romano
	UART Interface for Raspberry Pi (Broadcom 2835)
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include "uart.h"

#define QB_BUFSIZE 4096
struct QueueBuffer {
	struct QueueBufferNode {
		char buffer[QB_BUFSIZE];
		char *front, *back;
		struct QueueBufferNode *next;
	} *head, *tail;
};

static struct QueueBuffer *queuebuffer = 0;

/**
	Initialize a QueueBuffer.
*/
static void qb_initialize(struct QueueBuffer **qbuf);

/**
	Free resources used by QueueBuffer.
*/
static void qb_free(struct QueueBuffer *qbuf);

/**
	Add len bytes from the given buffer to the provided QueueBuffer.
*/
static void qb_push(struct QueueBuffer *qbuf, const void *buffer, size_t len);

/**
	Pop the next numbytes off the QueueBuffer into the given buffer.
	
	Returns the number of bytes actually popped. This could be less than
	numbytes if the number of bytes stored in the QueueBuffer is less than
	numbytes.
*/
static int qb_pop(struct QueueBuffer *qbuf, void *buffer, size_t numbytes);

/**
	Returns the number of bytes currently stored by qbuf.
*/
static int qb_getSize(struct QueueBuffer *qbuf);

// Internal error handling data
#define UART_ERRSIZE 128
static int error = 0;
static char error_str[UART_ERRSIZE];

static int uartfd = -1;

static void generateError(const char *str) {
	error = 1;
	strncpy(error_str, str, UART_ERRSIZE);
}

static void sigHandlerIO(int signumber);

int uart_init(int baudrate, UARTParity parity) {
	speed_t baud;
	switch (baudrate) {
		case 1200:
			baud = B1200;
			break;
		case 1800:
			baud = B1800;
			break;
		case 2400:
			baud = B2400;
			break;
		case 4800:
			baud = B4800;
			break;
		case 9600:
			baud = B9600;
			break;
		case 19200:
			baud = B19200;
			break;
		case 38400:
			baud = B38400;
			break;
		case 57600:
			baud = B57600;
			break;
		case 115200:
			baud = B115200;
			break;
		case 230400:
			baud = B230400;
			break;

		default:
			generateError("Unsupported baud rate requested");
			return 0;
	}

	uartfd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (uartfd == -1) {
		generateError("Could not open /dev/ttyAMA0");
		return 0;
	}

	struct sigaction sa;
	sa.sa_handler = sigHandlerIO;
	sa.sa_flags = 0;
	sa.sa_restorer = NULL;
	sigaction(SIGIO, &sa, NULL);

	fcntl(uartfd, F_SETOWN, getpid());
	fcntl(uartfd, F_SETFL, O_ASYNC);

	// Set terminal properties for /dev/ttyAMA0
	struct termios tprops;

	tprops.c_iflag = 0;
	if (parity != UART_PARDISABLE)
		tprops.c_iflag = INPCK;

	tprops.c_oflag = 0;

	tprops.c_cflag = CS8 | CREAD;
	if (parity != UART_PARDISABLE) {
		tprops.c_cflag |= PARENB;
		if (parity == UART_PARODD)
			tprops.c_cflag |= PARODD;
	}

	tprops.c_lflag = 0;

	// Disable control characters
	int cc;
	for (cc = 0; cc < NCCS; ++cc)
		tprops.c_cc[cc] = _POSIX_VDISABLE;

	tprops.c_cc[VMIN] = 0;  // No minimum number of characters for reads
	tprops.c_cc[VTIME] = 0; // No timeout (0 deciseconds)

	// Baud rate
	cfsetospeed(&tprops, baud);
	cfsetispeed(&tprops, baud);

	// Set the attributes
	tcsetattr(uartfd, TCSAFLUSH, &tprops);

	tcflow(uartfd, TCOON | TCION); // Restart input and output
	tcflush(uartfd, TCIOFLUSH);    // Flush buffer for clean start

	qb_initialize(queuebuffer);

	return 1;
}

int uart_deinit() {
	if (close(uartfd) != 0) {
		generateError("Could not close /dev/ttyAMA0");
		return 0;
	}
	return 1;
}

int uart_write(const void *buffer, size_t len) {
	if (uartfd == -1) {
		generateError("UART has not been initialized");
		return 0;
	}

	write(uartfd, buffer, len);
	return 1;
}

int uart_read(void *buffer, size_t len) {
	if (uartfd == -1) {
		generateError("UART has not been initialized");
		return 0;
	}

	return read(uartfd, buffer, len);
}

char *uart_getLastError() {
	if (error) {
		error = 0;
		return error_str;
	} else
		return "No error";
}

void sigHandlerIO(int signumber) {
	char buffer[51];
	int bytes;
	while ((bytes = read(uartfd, buffer, 50)) > 0) {
		buffer[bytes] = '\0';
		printf("%s", buffer);
	}
}

/**
	QueueBuffer
*/

void qb_initialize(struct QueueBuffer **qbuf) {
	*qbuf = malloc(sizeof(struct QueueBuffer));

	(*qbuf)->head = malloc(sizeof(struct QueueBufferNode));
	(*qbuf)->tail = (*qbuf)->head;

	(*qbuf)->head->front =
	(*qbuf)->head->back  = (*qbuf)->head->buffer;
	(*qbuf)->head->next  = NULL;
}

void qb_free(struct QueueBuffer **qbuf) {
	struct QueueBufferNode *current = (*qbuf)->head, temp;
	while (current != NULL) {
		temp = current->next;
		free(current);
		current = temp;
	}
	*qbuf = NULL;
}

void qb_push(struct QueueBuffer *qbuf, const void *buffer, size_t len) {
	const char *cbuffer = (const char*)buffer;
	int i = 0;

	while (i < len) {
		++qbuf->tail->back;

		// If the current node is full, add a new node to the back
		if (qbuf->tail->back - qbuf->tali->buffer >= QB_BUFSIZE) {
			qbuf->tail->next = malloc(sizeof(struct QueueBufferNode));
			qbuf->tail = qbuf->tail->next;

			qbuf->tail->front = 
			qbuf->tail->back  = qbuf->tail->buffer;
		}

		*qbuf->tail->back = cbuffer[i];
	}
}

int qb_pop(struct QueueBuffer *qbuf, void *buffer, size_t numbytes) {
	struct QueueBufferNode *current = qbuf->head;
	char *cbuffer = (char *)buffer;
	int i = 0;

	while (i < numbytes) {
		// Check if the head node is empty
		if (qbuf->head->front == qbuf->head->back) {
			if (qbuf->head->next == NULL) {
				// There is no more data; buffer is now empty
				return i;
			} else {
				// Move onto the next node, freeing the old head
				struct QueueBufferNode *temp = qbuf->head->next;
				free(qbuf->head);
				qbuf->head = temp;
			}
		}

		cbuffer[i] = qbuf->head->front;
		++qbuf->head->front;
	}

	return i;
}

int qb_getSize(struct QueueBuffer *qbuf) {
	
}

