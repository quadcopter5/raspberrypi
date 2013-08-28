CC = gcc
CFLAGS = -g

all: gpio_bare.x uart_bare.x twoway.x i2c.x i2c_control.x i2c_motor.x \
		i2c_sensor.x

gpio_bare.x: gpio_bare.c
	$(CC) $(CFLAGS) gpio_bare.c -o gpio_bare.x

uart_bare.x: uart_bare.c
	$(CC) $(CFLAGS) uart_bare.c -o uart_bare.x

twoway.x: twoway.c
	$(CC) $(CFLAGS) twoway.c -o twoway.x

i2c.x: i2c.c
	$(CC) $(CFLAGS) i2c.c -o i2c.x

i2c_control.x: i2c_control.c
	$(CC) $(CFLAGS) i2c_control.c -o i2c_control.x

i2c_motor.x: i2c_motor.c
	$(CC) $(CFLAGS) i2c_motor.c -o i2c_motor.x

i2c_sensor.x: i2c_sensor.c
	$(CC) $(CFLAGS) i2c_sensor.c -lm -o i2c_sensor.x


clean:
	rm -f *.o *.x

