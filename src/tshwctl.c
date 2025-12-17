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

#include "gpiod-helper.h"

#define STRAP0	23
#define STRAP1	24
#define STRAP2	27
#define STRAP3	38

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
	int i;
	struct gpiod_line_request *strap[4] = {NULL};
	int line[4] = {STRAP0, STRAP1, STRAP2, STRAP3};
	int ret = 0;
	enum gpiod_line_value value;

	strap[0] = request_input_line("/dev/gpiochip2", line[0], "tshwctl");
	strap[1] = request_input_line("/dev/gpiochip2", line[1], "tshwctl");
	strap[2] = request_input_line("/dev/gpiochip2", line[2], "tshwctl");
	strap[3] = request_input_line("/dev/gpiochip2", line[3], "tshwctl");
	if (strap[0] == NULL ||
	    strap[1] == NULL ||
	    strap[2] == NULL ||
	    strap[3] == NULL) {
		fprintf(stderr, "Unable to request GPIO pins for opts\n");
		ret = -1;
		goto out;
	}

	for (i = 0; i < 4; i++) {
		value = gpiod_line_request_get_value(strap[i], line[i]);
		if (value == GPIOD_LINE_VALUE_ERROR) {
			fprintf(stderr, "Error reading line %d\n", line[i]);
			ret = -1;
			goto out;
		}

		if (value == GPIOD_LINE_VALUE_ACTIVE)
			opts |= (1 << i);
	}

	printf("model=%X\n", get_model());
	printf("boardopt=%d\n", opts);

out:
	for (i = 0; i < 4; i++) {
		if (strap[i])
			gpiod_line_request_release(strap[i]);
	}

	return ret;
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
