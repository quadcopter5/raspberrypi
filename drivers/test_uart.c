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

	int done = 0;
	char buffer[50];
	while (!done) {
		printf("Tx: ");
		fflush(stdout);
		fgets(buffer, 50, stdin);

		int length = strlen(buffer);
		if (buffer[length - 1] == '\n') {
			--length;
			buffer[length] = '\0';
		}

		uart_write(buffer, length);
		if (strchr(buffer, '`') != NULL)
			done = 1;
	}

	// Read whenever RX FIFO is not empty, until a ` is read
	buffer[0] = 0;
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

