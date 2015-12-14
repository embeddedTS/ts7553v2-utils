/*  Copyright 2004-2012, Unpublished Work of Technologic Systems
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
*  gcc -fno-tree-cselim -Wall -O0 -mcpu=arm9 -o tshwctl tshwctl.c
*
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <linux/types.h>
#include <math.h>

#include "gpiolib.h"

static int twifd;

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

void auto485_en(int uart, int baud, char *mode)
{
	unsigned int *mxpwmregs = NULL;
	unsigned short speed;
	int devmem;

	devmem = open("/dev/mem", O_RDWR|O_SYNC);
	mxpwmregs = mmap(0, getpagesize(), PROT_READ|PROT_WRITE,
	  MAP_SHARED, devmem, 0x80064000);
	//3_18
	//mxgpioregs[0x178/4] = 0x3 << 4;
	
	speed = (24000000/((baud*1.275)-1));
	mxpwmregs[0x8/4] = 0x3<<30;
	mxpwmregs[0x4/4] = 1<<2;
	mxpwmregs[0x50/4] = (((speed/2)<<16));
	mxpwmregs[0x60/4] = (0x2<<18) | (0x3<<16) | speed;

	munmap((void *)mxpwmregs, getpagesize());
	close(devmem);
}


void usage(char **argv) {
	fprintf(stderr,
		"Usage: %s [OPTIONS] ...\n"
		"Technologic Systems I2C FPGA Utility\n"
		"\n"
		"  -i, --info             Display board info\n"
		"  -x, --baud <speed>     Set baud rate for auto485\n"
		"  -e, --cputemp          Print CPU internal temperature\n"
		"  -p, --getmac           Display ethernet MAC address\n"
		"  -l, --setmac=MAC       Set ethernet MAC address\n"
		"  -h, --help             This message\n"
		"\n",
		argv[0]
	);
}

int main(int argc, char **argv) 
{
	int c;
	int opt_info = 0, opt_setmac = 0, opt_getmac = 0;
	int opt_cputemp = 0;
	char *opt_mac = NULL;
	int baud = -1;
	int model;

	static struct option long_options[] = {
		{ "baud", 1, 0, 'x' },
		{ "getmac", 0, 0, 'p' },
		{ "setmac", 1, 0, 'l' },
		{ "cputemp", 0, 0, 'e' },
		{ "info", 0, 0, 'i' },
		{ "help", 0, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	model = get_model();
	if(model != 0x7553) {
		fprintf(stderr, "Unsupported model %d\n", model);
		return 1;
	}

	while((c = getopt_long(argc, argv, "x:hipl:e", long_options, NULL)) != -1) {
		switch(c) {

		case 'i':
			opt_info = 1;
			break;
		case 'e':
			opt_cputemp = 1;
			break;
		case 'x':
			baud = atoi(optarg);
			break;
		case 'l':
			opt_setmac = 1;
			opt_mac = strdup(optarg);
			break;
		case 'p':
			opt_getmac = 1;
			break;
		default:
			usage(argv);
			return 1;
		}
	}

	if(opt_info) {
		printf("model=0x%X\n", model);
		gpio_export(44);
		printf("bootmode=0x%X\n", gpio_read(44) ? 1 : 0);
		//printf("fpga_revision=0x%X\n", fpeek8(twifd, 0x7F));
	}

	if(baud > -1) {
		auto485_en(0, baud, NULL);
	}

	if (opt_cputemp) {
		signed int temp[2] = {0,0}, x;
		volatile unsigned int *mxlradcregs;
		int devmem;

		devmem = open("/dev/mem", O_RDWR|O_SYNC);
		assert(devmem != -1);
		mxlradcregs = (unsigned int *) mmap(0, getpagesize(),
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x80050000);

		mxlradcregs[0x148/4] = 0xFF;
		mxlradcregs[0x144/4] = 0x98; //Set to temp sense mode
		mxlradcregs[0x28/4] = 0x8300; //Enable temp sense block
		mxlradcregs[0x50/4] = 0x0; //Clear ch0 reg
		mxlradcregs[0x60/4] = 0x0; //Clear ch1 reg
		temp[0] = temp[1] = 0;

		for(x = 0; x < 10; x++) {
			/* Clear interrupts
			 * Schedule readings
			 * Poll for sample completion
			 * Pull out samples*/
			mxlradcregs[0x18/4] = 0x3;
			mxlradcregs[0x4/4] = 0x3;
			while(!((mxlradcregs[0x10/4] & 0x3) == 0x3)) ;
			temp[0] += mxlradcregs[0x60/4] & 0xFFFF;
			temp[1] += mxlradcregs[0x50/4] & 0xFFFF;
		}
		temp[0] = (((temp[0] - temp[1]) * (1012/4)) - 2730000);
		printf("internal_temp=%d.%d\n",temp[0] / 10000,
		  abs(temp[0] % 10000));

		munmap((void *)mxlradcregs, getpagesize());
		close(devmem);
	}

	if (opt_setmac) {
		/* This uses one time programmable memory. */
		unsigned int a, b, c;
		int r, devmem;
		volatile unsigned int *mxocotpregs;

		devmem = open("/dev/mem", O_RDWR|O_SYNC);
		assert(devmem != -1);

		r = sscanf(opt_mac, "%*x:%*x:%*x:%x:%x:%x",  &a,&b,&c);
		assert(r == 3); /* XXX: user arg problem */

		mxocotpregs = (unsigned int *) mmap(0, getpagesize(),
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x8002C000);

		mxocotpregs[0x08/4] = 0x200;
		mxocotpregs[0x0/4] = 0x1000;
		while(mxocotpregs[0x0/4] & 0x100) ; //check busy flag
		if(mxocotpregs[0x20/4] & (0xFFFFFF)) {
			printf("MAC address previously set, cannot set\n");
		} else {
			assert(a < 0x100);
			assert(b < 0x100);
			assert(c < 0x100);
			mxocotpregs[0x0/4] = 0x3E770000;
			mxocotpregs[0x10/4] = (a<<16|b<<8|c);
		}
		mxocotpregs[0x0/4] = 0x0;

		munmap((void *)mxocotpregs, getpagesize());
		close(devmem);
	}

	if (opt_getmac) {
		unsigned char a, b, c;
		unsigned int mac;
		int devmem;
		volatile unsigned int *mxocotpregs;

		devmem = open("/dev/mem", O_RDWR|O_SYNC);
		assert(devmem != -1);
		mxocotpregs = (unsigned int *) mmap(0, getpagesize(),
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x8002C000);

		mxocotpregs[0x08/4] = 0x200;
		mxocotpregs[0x0/4] = 0x1000;
		while(mxocotpregs[0x0/4] & 0x100) ; //check busy flag
		mac = mxocotpregs[0x20/4] & 0xFFFFFF;
		if(!mac) {
			mxocotpregs[0x0/4] = 0x0; //Close the reg first
			mxocotpregs[0x08/4] = 0x200;
			mxocotpregs[0x0/4] = 0x1013;
			while(mxocotpregs[0x0/4] & 0x100) ; //check busy flag
			mac = (unsigned short) mxocotpregs[0x150/4];
			mac |= 0x4f0000;
		}
		mxocotpregs[0x0/4] = 0x0;

		a = mac >> 16;
		b = mac >> 8;
		c = mac;
		
		printf("mac=00:d0:69:%02x:%02x:%02x\n", a, b, c);
		printf("shortmac=%02x%02x%02x\n", a, b, c);

		munmap((void *)mxocotpregs, getpagesize());
		close(devmem);
	}


	close(twifd);

	return 0;
}

