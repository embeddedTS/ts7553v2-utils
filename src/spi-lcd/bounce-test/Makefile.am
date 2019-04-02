AUTOMAKE_OPTIONS = subdir-objects
GITCOMMIT:= $(shell git describe --abbrev=12 --dirty --always)

bounce_test_SOURCES = main.c ../cairo-test/lcd-display.c
bounce_test_CFLAGS = -O3 -Wall -I../cairo-test/
bounce_test_CPPFLAGS = -DGITCOMMIT="\"${GITCOMMIT}\""

bin_PROGRAMS = bounce-test
