PROGNAME = keypad-test

PWD		= $(shell pwd)
 
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc
 
OBJS := main.o lcd-display.o

INCLUDES = -I/usr/include -I/usr/include/cairo -I../spi-lcd/cairo-test
LIBS = -lcairo

CFLAGS = $(INCLUDES) -Wall -O3
LDFLAGS = $(LIBS)

vpath %.c ../spi-lcd/cairo-test

$(PROGNAME):  $(OBJS)	
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.obj : %.c 
	@$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
   
clean:
	rm -f $(OBJS) $(PROGNAME)


