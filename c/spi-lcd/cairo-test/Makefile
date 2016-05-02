PROGNAME = cairo-test

PWD		= $(shell pwd)
CROSS_COMPILE	?= $(PWD)/../../../cross-toolchain/glibc/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-none-linux-gnueabi-

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc


OBJS := main.o lcd-display.o

INCLUDES = -I/usr/include -I/usr/include/cairo
LIBS = -lcairo
CFLAGS = $(INCLUDES) -Wall -O3
LDFLAGS = $(LIBS)
	
$(PROGNAME):  $(OBJS)	
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.obj : %.c 
	@$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
   
clean:
	rm -f $(OBJS) $(PROGNAME)


