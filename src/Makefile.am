GITCOMMIT:= $(shell git describe --abbrev=12 --dirty --always)

tshwctl_SOURCES = tshwctl.c gpiod-helper.c
tshwctl_CFLAGS = -Wall
tshwctl_CPPFLAGS = -DGITCOMMIT="\"${GITCOMMIT}\""
tshwctl_LDADD = -lgpiod

tsmicroctl_SOURCES = tsmicroctl.c
tsmicroctl_CFLAGS = -Wall
tsmicroctl_CPPFLAGS = -DCTL -DGITCOMMIT="\"${GITCOMMIT}\""

bin_PROGRAMS = tshwctl tsmicroctl
