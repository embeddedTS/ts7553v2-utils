GITCOMMIT:= $(shell git describe --abbrev=12 --dirty --always)

cairo_test_SOURCES = main.c lcd-display.c
cairo_test_CFLAGS = -O3 -Wall
cairo_test_CPPFLAGS = -DGITCOMMIT="\"${GITCOMMIT}\""
cairo_test_LDFLAGS = -lcairo

bin_PROGRAMS = cairo-test
