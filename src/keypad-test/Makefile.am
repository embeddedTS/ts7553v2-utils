AUTOMAKE_OPTIONS = subdir-objects

keypad_test_SOURCES = main.c ../spi-lcd/cairo-test/lcd-display.c
keypad_test_CFLAGS = -O3 -Wall -I../spi-lcd/cairo-test/
keypad_test_LDFLAGS = -lcairo

bin_PROGRAMS = keypad-test
