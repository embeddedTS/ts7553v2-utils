/*  Copyright 2016, Unpublished Work of Technologic Systems
*  All Rights Reserved.
*
*  THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
*  PROPRIETARY AND TRADE SECRET INFORMATION OF TECHNOLOGIC SYSTEMS.
*  ACCESS TO THIS WORK IS RESTRICTED TO (I) TECHNOLOGIC SYSTEMS EMPLOYEES
*  WHO HAVE A NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
*  ASSIGNMENTS  AND (II) ENTITIES OTHER THAN TECHNOLOGIC SYSTEMS WHO
*  HAVE ENTERED INTO  APPROPRIATE LICENSE AGREEMENTS.  NO PART OF THIS
*  WORK MAY BE USED, PRACTICED, PERFORMED, COPIED, DISTRIBUTED, REVISED,
*  MODIFIED, TRANSLATED, ABRIDGED, CONDENSED, EXPANDED, COLLECTED,
*  COMPILED,LINKED,RECAST, TRANSFORMED, ADAPTED IN ANY FORM OR BY ANY
*  MEANS,MANUAL, MECHANICAL, CHEMICAL, ELECTRICAL, ELECTRONIC, OPTICAL,
*  BIOLOGICAL, OR OTHERWISE WITHOUT THE PRIOR WRITTEN PERMISSION AND
*  CONSENT OF TECHNOLOGIC SYSTEMS . ANY USE OR EXPLOITATION OF THIS WORK
*  WITHOUT THE PRIOR WRITTEN CONSENT OF TECHNOLOGIC SYSTEMS  COULD
*  SUBJECT THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.
*/
/* To compile tshwctl, use the appropriate cross compiler and run the
* command:
*
*  gcc -fno-tree-cselim -Wall -O0 -mcpu=arm9 -DCTL -o tsmicroctl tsmicroctl.c
*
*/

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

const char copyright[] = "Copyright (c) Technologic Systems - " __DATE__ ;

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
			perror("Microcontroller did not ACK 0x78\n");
			return -1;
		}
	}

	return fd;
}

#ifdef CTL

void do_info(int twifd)
{
	uint8_t data[28];
	bzero(data, 28);
	read(twifd, data, 28);

	printf("revision=0x%x\n", (data[16] >> 4) & 0xF);
	printf("P1_2=0x%x\n", data[0]<<8|data[1]);
	printf("P1_3=0x%x\n", data[2]<<8|data[3]);
	printf("P1_4=0x%x\n", data[4]<<8|data[5]);

	printf("P2_0=0x%x\n", data[6]<<8|data[7]);
	printf("P2_1=0x%x\n", data[8]<<8|data[9]);
	printf("P2_2=0x%x\n", data[10]<<8|data[11]);
	printf("P2_3=0x%x\n", data[12]<<8|data[13]);
	printf("P2_4=0x%x\n", data[18]<<8|data[19]);
	printf("P2_5=0x%x\n", data[20]<<8|data[21]);
	printf("P2_6=0x%x\n", data[22]<<8|data[23]);
	printf("P2_7=0x%x\n", data[24]<<8|data[25]);

	printf("temp_sensor=0x%x\n", data[26]<<8|data[27]);
	printf("reboot_source=");
	switch(data[15] & 0x7) {
	  case 0:
		printf("poweron\n");
		break;
	  case 1:
		printf("WDT\n");
		break;
	  case 2:
		printf("resetswitch\n");
		break;
	  case 3:
		printf("sleep\n");
		break;
	  case 4:
		printf("brownout\n");
		break;
	}

 
}

static void usage(char **argv) {
	fprintf(stderr, "Usage: %s [OPTION] ...\n"
	  "Technologic Systems Microcontroller Access\n"
	  "\n"
	  "  -i, --info              Get info about the microcontroller\n"
	  "  -L, --sleep             Sleep CPU, must specify -M|-m to wake\n"
	  /*"  -F, --standby           Standby CPU, must specify -M|-m to wake\n"*/
	  "  -M, --timewkup=<time>   Time in seconds to wake up after\n"
	  "  -m, --resetswitchwkup   Wake up at reset switch is press\n"
	  "  -X, --resetswitchon     Enable reset switch\n"
	  "  -Y, --resetswitchoff    Disable reset switch\n"
	  "  -h, --help              This message\n",
	  argv[0]
	);
}

int main(int argc, char **argv)
{
	int c;
	int twifd;
	int opt_resetswitch = 0, opt_sleepmode = 0, opt_timewkup = 0xffffff;
	int opt_resetswitchwkup = 0;

	static struct option long_options[] = {
	  { "info", 0, 0, 'i' },
	  { "sleep", 0, 0, 'L'},
	  /*{ "standby", 0, 0, 'F'},*/
	  { "timewkup", 1, 0, 'M'},
	  { "resetswitchwkup", 0, 0, 'm'},
	  { "resetswitchon", 0, 0, 'X'},
	  { "resetswitchoff", 0, 0, 'Y'},
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
	  case 0x7682:
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

	

	while((c = getopt_long(argc, argv, 
	  "iLFM:XYhm",
	  long_options, NULL)) != -1) {
		switch (c) {
		  case 'i':
			do_info(twifd);
			break;
		  case 'L':
			opt_sleepmode = 1;
			break;
		  /* Standby is currently not supported by kernel*/
		  /*case 'F':
			opt_sleepmode = 2;
			break;*/
		  case 'M':
			opt_timewkup = strtoul(optarg, NULL, 0);
			break;
		  case 'm':
			opt_resetswitchwkup = 1;
			break;
		  case 'X':
			opt_resetswitch = 2;
			break;
		  case 'Y':
			opt_resetswitch = 1;
			break;
		  case 'h':
		  default:
			usage(argv);
			return 1;
		}
	}

	if(opt_resetswitch) {
		unsigned char dat = 0x40;	
		
		dat |= (opt_resetswitch & 0x2); //0x40 for off, 0x42 on
		write(twifd, &dat, 1);
	}

	if(opt_sleepmode) {
		unsigned char dat[4] = {0};

		dat[0]=(0x1 | (opt_resetswitchwkup << 1) |
		  ((opt_sleepmode-1) << 4) | 1 << 6);
		dat[3] = (opt_timewkup & 0xff);
		dat[2] = ((opt_timewkup >> 8) & 0xff);
		dat[1] = ((opt_timewkup >> 16) & 0xff);
		write(twifd, &dat, 4);
	}

	
	return 0;
}

#endif
