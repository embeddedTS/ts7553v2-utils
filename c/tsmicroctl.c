#include <stdio.h>
#include <unistd.h>
#include <dirent.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef CTL
#include <getopt.h>
#endif

#include "i2c-dev.h"

#ifdef CTL
int model = 0;

int get_model()
{
	FILE *proc;
	char mdl[256];
	char *ptr;
	int sz;

	proc = fopen("/proc/device-tree/model", "r");
	if (!proc) {
	    perror("model");
	    return 0;
	}
	sz = fread(mdl, 256, 1, proc);
	ptr = strstr(mdl, "TS-");
	return strtoull(ptr+3, NULL, 16);
}
#endif

int silabs_init()
{
	static int fd = -1;
	fd = open("/dev/i2c-0", O_RDWR);
	if(fd != -1) {
		if (ioctl(fd, I2C_SLAVE_FORCE, 0x78) < 0) {
			perror("FPGA did not ACK 0x78\n");
			return -1;
		}
	}

	return fd;
}

// Scale voltage to silabs 0-2.5V
uint16_t inline sscale(uint16_t data){
	return data * 2.5 * 1024/1000;
}

// Scale voltage for resistor dividers
uint16_t inline rscale(uint16_t data, uint16_t r1, uint16_t r2)
{
	uint16_t ret = (data * (r1 + r2)/r2);
	return sscale(ret);
}

#ifdef CTL

void do_info(int twifd)
{
	uint8_t data[17];
	bzero(data, 17);
	read(twifd, data, 17);

	printf("revision=0x%x\n", (data[16] >> 4) & 0xF);
}

static void usage(char **argv) {
	fprintf(stderr, "Usage: %s [OPTION] ...\n"
	  "Technologic Systems Microcontroller Access\n"
	  "\n"
	  "  -h, --help            This message\n"
	  "  -i, --info            Get info about the microcontroller\n",
	  argv[0]
	);
}

int main(int argc, char **argv)
{
	int c;
	int twifd;

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
	  case 0x7680:
	  case 0x7400:
	  case 0x7553:
		break;
	  default:
		fprintf(stderr, "Unsupported model TS-%x\n", model);
		return 1;
	}

	twifd = silabs_init();
	if(twifd == -1)
	  return 1;

	

	while((c = getopt_long(argc, argv, "ih", long_options, NULL)) != -1) {
		switch (c) {
		case 'i':
			do_info(twifd);
			break;
		case 'h':
		default:
			usage(argv);
		}
	}

	return 0;
}

#endif
