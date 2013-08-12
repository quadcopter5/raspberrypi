/**
	Philip Romano
	Test for UART
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "uart.h"

static void ignoreInput();

int main(int argc, char **argv) {
	if (!uart_init(9600, UART_PARDISABLE)) {
		fprintf(stderr, "uart_init(): %s\n", uart_getLastError());
		return -1;
	}

	int  done = 0,
	     bytes;
	char buffer[51],
	     *pos;

	while (!done) {
		// Receive
		bytes = uart_read(buffer, 50);
		buffer[bytes] = '\0';
		if (bytes > 0) {
			// Convert CR to LF
			char *pos;
			while ((pos = strchr(buffer, '\r')) != NULL)
				*pos = '\n';
			printf("Rx: %s\n", buffer);
		}
		bytes = uart_getInputQueueSize();
		printf("(%d bytes in queue)\n", bytes);

		if (strchr(buffer, '`') != NULL)
			done = 1;
		else {
			// Transmit
			printf("Tx: ");
			int gotten = 0;
			while (fgets(buffer, 50, stdin) == NULL);

			bytes = strlen(buffer);
			if (!(bytes == 1 && buffer[0] == '\n')) {
				while ((pos = strchr(buffer, '\n')) != NULL)
					*pos = '\r';

				uart_write(buffer, bytes);
				if (strchr(buffer, '`') != NULL)
					done = 1;
			}
		}
	}

	/*
	// Read whenever RX FIFO is not empty, until a ` is read
	done = 0;
	int bytes;

	printf("Waiting...\n");
	while (!done) {
		bytes = uart_read(buffer, 49);

		if (bytes > 0) {
			buffer[bytes] = '\0'; // Terminate string

			printf("Rx: %s\n", buffer);
			printf("Waiting...\n");

			if (strchr(buffer, '`') != NULL)
				done = 1;
		}
	}
	*/

	if (!uart_deinit()) {
		fprintf(stderr, "uart_deinit(): %s\n", uart_getLastError());
		return -1;
	}

	printf("Done!\n");
	return 0;
}

void ignoreInput() {
	char c;
	while ((c = getchar()) != '\n' && c != EOF);
}

