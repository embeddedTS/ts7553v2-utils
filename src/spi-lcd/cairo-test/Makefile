PROGNAME = cairo-test

PWD		= $(shell pwd)

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


