GITCOMMIT:= $(shell git describe --abbrev=12 --dirty --always)

tshwctl_SOURCES = tshwctl.c gpiolib.c
tshwctl_CFLAGS = -O0 -Wall -fno-tree-cselim
tshwctl_CPPFLAGS = -DGITCOMMIT="\"${GITCOMMIT}\""

tsmicroctl_SOURCES = tsmicroctl.c
tsmicroctl_CFLAGS = -O0 -Wall -fno-tree-cselim
tsmicroctl_CPPFLAGS = -DCTL -DGITCOMMIT="\"${GITCOMMIT}\""

gpioctl_SOURCES = gpiolib.c
gpioctl_CFLAGS = -O0 -Wall -fno-tree-cselim
gpioctl_CPPFLAGS = -DCTL -DGITCOMMIT="\"${GITCOMMIT}\""

bin_PROGRAMS = tshwctl tsmicroctl gpioctl
