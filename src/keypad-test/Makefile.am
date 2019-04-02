AUTOMAKE_OPTIONS = subdir-objects
GITCOMMIT:= $(shell git describe --abbrev=12 --dirty --always)

keypad_test_SOURCES = main.c ../spi-lcd/cairo-test/lcd-display.c
keypad_test_CFLAGS = -O3 -Wall -I../spi-lcd/cairo-test/
keypad_test_CPPFLAGS = -DGITCOMMIT="\"${GITCOMMIT}\""
keypad_test_LDFLAGS = -lcairo

bin_PROGRAMS = keypad-test
