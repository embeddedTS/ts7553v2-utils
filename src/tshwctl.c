/* SPDX-License-Identifier: BSD-2-Clause */

#include <stdio.h>
#include <unistd.h>
#include <dirent.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <gpiod.h>

#define STRAP0 87
#define STRAP1 88
#define STRAP2 91
#define STRAP3 92

const char copyright[] = "Copyright (c) embeddedTS - " __DATE__ " - "
  GITCOMMIT;

int model = 0;

int get_model()
{
	FILE *proc;
	char mdl[256];
	char *ptr;

	proc = fopen("/proc/device-tree/model", "r");
	if (!proc) {
	    perror("model");
	    return 0;
	}
	fread(mdl, 256, 1, proc);
	ptr = strstr(mdl, "TS-");
	return strtoull(ptr+3, NULL, 16);
}

int do_info(void)
{
	uint8_t opts = 0;
	struct gpiod_chip *chip2 = NULL;
	struct gpiod_line *strap0 = NULL;
	struct gpiod_line *strap1 = NULL;
	struct gpiod_line *strap2 = NULL;
	struct gpiod_line *strap3 = NULL;

	chip2 = gpiod_chip_open_by_number(2);
	if (chip2 == NULL) {
		fprintf(stderr, "Unable to open gpio chip 2");
		return -1;
	}
	strap0 = gpiod_chip_get_line(chip2, 23);
	strap1 = gpiod_chip_get_line(chip2, 24);
	strap2 = gpiod_chip_get_line(chip2, 27);
	strap3 = gpiod_chip_get_line(chip2, 28);
	if (strap0 == NULL ||
	    strap1 == NULL ||
	    strap2 == NULL ||
	    strap3 == NULL) {
		fprintf(stderr, "Unable find GPIO pins for opts\n");
		return -1;
	}

	if (gpiod_line_request_input(strap0, "tshwctl") ||
	    gpiod_line_request_input(strap1, "tshwctl") ||
	    gpiod_line_request_input(strap2, "tshwctl") ||
	    gpiod_line_request_input(strap3, "tshwctl")) {
		fprintf(stderr, "Unable to request GPIO pins for opts\n");
		return -1;
	}

	opts |= (gpiod_line_get_value(strap0) << 0);
	opts |= (gpiod_line_get_value(strap1) << 1);
	opts |= (gpiod_line_get_value(strap2) << 2);
	opts |= (gpiod_line_get_value(strap3) << 3);

	gpiod_line_release(strap0);
	gpiod_line_release(strap1);
	gpiod_line_release(strap2);
	gpiod_line_release(strap3);
	gpiod_chip_close(chip2);

	printf("model=%X\n", get_model());
	printf("boardopt=%d\n", opts);

	return 0;

}

static void usage(char **argv) {
	fprintf(stderr,
	  "%s\n\n"
	  "Usage: %s [OPTION] ...\n"
	  "embeddedTS Hardware access\n"
	  "\n"
	  "  -i, --info              Get info about the SBC\n"
	  "  -h, --help              This message\n"
	  "\n",
	  copyright, argv[0]
	);
}

int main(int argc, char **argv)
{
	int c;
	int ret = 1;

	static struct option long_options[] = {
	  { "info", 0, 0, 'i' },
	  { "help", 0, 0, 'h' },
	  { 0, 0, 0, 0 }
	};

	if(argc == 1) {
		usage(argv);
		return(1);
	}

	model = get_model();
	switch(model) {
	  case 0x7553:
		break;
	  default:
		fprintf(stderr, "Unsupported model TS-%x\n", model);
		return 1;
	}

	while((c = getopt_long(argc, argv, 
	  "ih",
	  long_options, NULL)) != -1) {
		switch (c) {
		  case 'i':
			ret = do_info();
			break;
		  case 'h':
		  default:
			usage(argv);
			return 1;
		}
	}

	return (ret ? 1 : 0);
}
