# Makefile for Raspberry Pi peripheral drivers

CC = gcc
CFLAGS =
DEBUGFLAGS = -g -D_DEBUG

OBJDIR = obj
LIBDIR = lib
BINDIR = bin


all: dirs $(LIBDIR)/peripherals.a tests

debug: dirs $(LIBDIR)/peripherals_d.a


dirs:
	mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR)

# Driver archive
$(LIBDIR)/peripherals.a: $(OBJDIR)/gpio.o $(OBJDIR)/uart.o $(OBJDIR)/queuebuffer.o
	ar r $(LIBDIR)/peripherals.a $(OBJDIR)/gpio.o $(OBJDIR)/uart.o $(OBJDIR)/queuebuffer.o

tests: $(BINDIR)/test_gpio.x $(BINDIR)/test_uart.x $(BINDIR)/test_queuebuffer.x

$(LIBDIR)/peripherals_d.a: $(OBJDIR)/gpio_d.o $(OBJDIR)/uart_d.o $(OBJDIR)/queuebuffer_d.o
	ar r $(LIBDIR)/peripherals_d.a $(OBJDIR)/gpio_d.o $(OBJDIR)/uart_d.o \
		$(OBJDIR)/queuebuffer_d.o


# Driver object files

$(OBJDIR)/gpio.o: gpio.c gpio.h
	$(CC) $(CFLAGS) -c gpio.c -o $(OBJDIR)/gpio.o

$(OBJDIR)/uart.o: uart.c uart.h
	$(CC) $(CFLAGS) -c uart.c -o $(OBJDIR)/uart.o

$(OBJDIR)/queuebuffer.o: queuebuffer.c queuebuffer.h
	$(CC) $(CFLAGS) -c queuebuffer.c -o $(OBJDIR)/queuebuffer.o


$(OBJDIR)/gpio_d.o: gpio.c gpio.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -c gpio.c -o $(OBJDIR)/gpio_d.o

$(OBJDIR)/uart_d.o: uart.c uart.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -c uart.c -o $(OBJDIR)/uart_d.o

$(OBJDIR)/queuebuffer_d.o: queuebuffer.c queuebuffer.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -c queuebuffer.c -o $(OBJDIR)/queuebuffer_d.o

# Tests

$(BINDIR)/test_gpio.x: $(OBJDIR)/test_gpio.o $(OBJDIR)/gpio.o
	$(CC) $(OBJDIR)/test_gpio.o $(OBJDIR)/gpio.o -o $(BINDIR)/test_gpio.x

$(OBJDIR)/test_gpio.o: test_gpio.c gpio.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -c test_gpio.c -o $(OBJDIR)/test_gpio.o

$(BINDIR)/test_uart.x: $(OBJDIR)/test_uart.o $(OBJDIR)/uart.o $(OBJDIR)/queuebuffer.o
	$(CC) $(OBJDIR)/test_uart.o $(OBJDIR)/uart.o $(OBJDIR)/queuebuffer.o \
		-o $(BINDIR)/test_uart.x

$(OBJDIR)/test_uart.o: test_uart.c uart.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -c test_uart.c -o $(OBJDIR)/test_uart.o

$(BINDIR)/test_queuebuffer.x: $(OBJDIR)/test_queuebuffer.o $(OBJDIR)/queuebuffer.o
	$(CC) $(OBJDIR)/test_queuebuffer.o $(OBJDIR)/queuebuffer.o \
		-o $(BINDIR)/test_queuebuffer.x

$(OBJDIR)/test_queuebuffer.o: test_queuebuffer.c queuebuffer.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -c test_queuebuffer.c -o $(OBJDIR)/test_queuebuffer.o


clean:
	rm -rf $(OBJDIR) $(BINDIR) $(LIBDIR) *.o *.x *.a

