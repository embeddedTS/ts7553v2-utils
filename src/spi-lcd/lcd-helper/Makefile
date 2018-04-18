PROGNAME = lcd-helper

PWD		= $(shell pwd)

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc

INCLUDES +=  -I$(PWD)/../../
vpath %.c $(PWD)/../../

OBJS := spi-lcd.o 

CFLAGS = $(INCLUDES) -Wall -O3
	
$(PROGNAME):  $(OBJS)	
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.obj : %.c
	@$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
   
clean:
	rm -f $(OBJS) $(PROGNAME)


