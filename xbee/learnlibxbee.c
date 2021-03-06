/*
	8/8/2013
	Learning libxbee API
*/

#include <stdio.h>
#include <string.h>
#include <xbee.h>

int main(int argc, char **argv) {
	struct xbee     *xbee_instance;
	struct xbee_con *xbee_connection;
	xbee_err        xberror;
	int             baudrate;

	baudrate = 57600;
	if (argc > 1) {
		// Get baud rate from argument
		baudrate = atoi(argv[1]);
		if (baudrate <= 0) {
			baudrate = 57600;
			fprintf(stderr, "Invalid baud rate given\n");
			return 1;
		}
	}

	// Set up libxbee
#if defined(_WIN32)
	xberror = xbee_setup(&xbee_instance, "xbee1", "COM3", baudrate, 2);
#else
	xberror = xbee_setup(&xbee_instance, "xbee1", "/dev/ttyUSB0", baudrate, 2);
#endif
	if (xberror != XBEE_ENONE) {
		fprintf(stderr, "xbee_setup(): (Error %d) %s\n", xberror,
				xbee_errorToStr(xberror));
		return 1;
	}
	printf("XBee set up, using baud rate %d\n", baudrate);

	// Set up connection
	xberror = xbee_conNew(xbee_instance, &xbee_connection, "Local AT", NULL);
	if (xberror != XBEE_ENONE) {
		fprintf(stderr, "xbee_conNew(): (Error %d) %s\n", xberror,
				xbee_errorToStr(xberror));
		return 1;
	}
	printf("XBee connection created using Local AT\n");

	// Disable ACK (don't wait for response)
	// This is just while we don't have the remote XBee module communicating
	struct xbee_conSettings xbsettings;
	xberror = xbee_conSettings(xbee_connection, NULL, &xbsettings);
	if (xberror != XBEE_ENONE) {
		fprintf(stderr, "xbee_conSettings(): (Error %d) %s\n", xberror,
				xbee_errorToStr(xberror));
		return 1;
	}

	xbsettings.disableAck = 1;

	xberror = xbee_conSettings(xbee_connection, &xbsettings, NULL);
	if (xberror != XBEE_ENONE) {
		fprintf(stderr, "xbee_conSettings(): (Error %d) %s\n", xberror,
				xbee_errorToStr(xberror));
		return 1;
	}
	printf("Disabled ACK\n");

	xberror = xbee_conValidate(xbee_connection);
	if (xberror != XBEE_ENONE) {
		fprintf(stderr, "xbee_conValidate(): (Error %d) %s\n", xberror,
				xbee_errorToStr(xberror));
		return 1;
	}
	printf("XBee connection validated\n");

	// Transmit data
	char buffer[50];
	unsigned char retval;
	int done = 0;
	while (!done) {
		fgets(buffer, 50, stdin);
		if (buffer[strlen(buffer) - 1] == '\n')
			buffer[strlen(buffer) - 1] = '\0';

		if (strcmp(buffer, "q") == 0)
			done = 1;
		else {
			xberror = xbee_conTx(xbee_connection, &retval, buffer);
			if (xberror != XBEE_ENONE) {
				if (retval == XBEE_ETX)
					fprintf(stderr, "Transmission error occurred: 0x%02X\n", retval);
				else
					fprintf(stderr, "xbee_connTx(): (Error %d) %s\n", xberror,
							xbee_errorToStr(xberror));
				return 1;
			}
			printf("Data transmitted. Return value 0x%02X\n", retval);
		}
	}

	// Receive data
	printf("Waiting to receive data...\n");
	int numpkts;
	struct xbee_pkt *pkt;
	
	xberror = xbee_conRxWait(xbee_connection, &pkt, &numpkts);
	if (xberror != XBEE_ENONE) {
		fprintf(stderr, "xbee_conRx(): (Error %d) %s\n", xberror,
				xbee_errorToStr(xberror));
		return 1;
	}
	printf("%d packets in buffer\n", numpkts);

	// End connection
	xberror = xbee_conEnd(xbee_connection);
	if (xberror != XBEE_ENONE) {
		fprintf(stderr, "xbee_conEnd(): (Error %d) %s\n", xberror,
				xbee_errorToStr(xberror));
		return 1;
	}
	printf("XBee connection ended\n");

	// Shut down libxbee
	xberror = xbee_shutdown(xbee_instance);
	if (xberror != XBEE_ENONE) {
		fprintf(stderr, "xbee_shutdown(): (Error %d) %s\n", xberror,
				xbee_errorToStr(xberror));
		return 1;
	}
	printf("XBee shut down\n");

	return 0;
}

