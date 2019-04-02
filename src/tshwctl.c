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
#include "gpiolib.h"

#define STRAP0 87
#define STRAP1 88
#define STRAP2 91
#define STRAP3 92

const char copyright[] = "Copyright (c) Technologic Systems - " __DATE__ " - "
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
	int ret = 0;

	ret += gpio_export(STRAP0);
	ret += gpio_export(STRAP1);
	ret += gpio_export(STRAP2);
	ret += gpio_export(STRAP3);

	if (ret < 0) {
		fprintf(stderr, "Unable to access GPIO pins for board opts!\n");
	} else {
		opts |= (gpio_read(STRAP0) << 0);
		opts |= (gpio_read(STRAP1) << 1);
		opts |= (gpio_read(STRAP2) << 2);
		opts |= (gpio_read(STRAP3) << 3);

		printf("model=%X\n", get_model());
		printf("boardopt=%d\n", opts);
	}

	return ret;

}

static void usage(char **argv) {
	fprintf(stderr,
	  "%s\n\n"
	  "Usage: %s [OPTION] ...\n"
	  "Technologic Systems Hardware access\n"
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
