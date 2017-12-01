OBJS = touch_i2c.o sdram.o clock.o console.o lcd-spi.o gfx.o gpio.o

BINARY = lcd-serial-touch

# we use sin/cos from the library
LDLIBS += -lm

LDSCRIPT = ../stm32f429i-discovery.ld

include ../../Makefile.include
