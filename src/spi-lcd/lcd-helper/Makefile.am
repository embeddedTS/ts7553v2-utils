GITCOMMIT:= $(shell git describe --abbrev=12 --dirty --always)

lcd_helper_SOURCES = spi-lcd.c
lcd_helper_CFLAGS = -O3 -Wall
lcd_helper_CPPFLAGS = -DGITCOMMIT="\"${GITCOMMIT}\""

bin_PROGRAMS = lcd-helper
