AUTOMAKE_OPTIONS = subdir-objects
GITCOMMIT:= $(shell git describe --abbrev=12 --dirty --always)

lcd_helper_SOURCES = spi-lcd.c ../../gpiod-helper.c
lcd_helper_CFLAGS = -O3 -Wall
lcd_helper_CPPFLAGS = -DGITCOMMIT="\"${GITCOMMIT}\""
lcd_helper_LDADD = -lgpiod

bin_PROGRAMS = lcd-helper
