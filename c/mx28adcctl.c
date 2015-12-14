/*  Copyright 2015, Unpublished Work of Technologic Systems
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
/* To compile mx28adcctl, use the appropriate cross compiler and run the
* command:
*
*  gcc -fno-tree-cselim -Wall -O0 -mcpu=arm9 -o mx28adcctl mx28adcctl.c
*/

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


const char copyright[] = "Copyright (c) Technologic Systems - " __DATE__ ;

int main(int argc, char **argv) {
	volatile unsigned int *mxlradcregs;
	volatile unsigned int *mxhsadcregs;
	volatile unsigned int *mxclkctrlregs;
	unsigned int i, x, chan[8] = {0,0,0,0,0,0,0,0};
	//signed int bivolt;
	int devmem;

	devmem = open("/dev/mem", O_RDWR|O_SYNC);
	assert(devmem != -1);

	// LRADC
	mxlradcregs = (unsigned int *) mmap(0, getpagesize(),
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x80050000);

	mxlradcregs[0x148/4] = 0xfffffff; //Clear LRADC6:0 assignments
	mxlradcregs[0x144/4] = 0x6543210; //Set LRDAC6:0 to channel 6:0
	mxlradcregs[0x28/4] = 0xff000000; //Set 1.8v range
	for(x = 0; x < 7; x++)
	  mxlradcregs[(0x50+(x * 0x10))/4] = 0x0; //Clear LRADCx reg

	for(x = 0; x < 10; x++) {
		mxlradcregs[0x18/4] = 0x7f; //Clear interrupt ready
		mxlradcregs[0x4/4] = 0x7f; //Schedule conversaion of chan 6:0
		while(!((mxlradcregs[0x10/4] & 0x7f) == 0x7f)); //Wait
		for(i = 0; i < 7; i++)
		  chan[i] += (mxlradcregs[(0x50+(i * 0x10))/4] & 0xffff);
	}

	mxhsadcregs = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED,
	  devmem, 0x80002000);
	mxclkctrlregs = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED,
	  devmem, 0x80040000);

	// HDADC
	//Lets see if we need to bring the HSADC out of reset
	if(mxhsadcregs[0x0/4] & 0xC0000000) {
		mxclkctrlregs[0x154/4] = 0x70000000;
		mxclkctrlregs[0x1c8/4] = 0x8000;
		//ENGR116296 errata workaround
		mxhsadcregs[0x8/4] = 0x80000000;
		mxhsadcregs[0x0/4] = ((mxhsadcregs[0x0/4] | 0x80000000) & (~0x40000000));
		mxhsadcregs[0x4/4] = 0x40000000;
		mxhsadcregs[0x8/4] = 0x40000000;
		mxhsadcregs[0x4/4] = 0x40000000;

		usleep(10);
		mxhsadcregs[0x8/4] = 0xc0000000;
	}

	mxhsadcregs[0x28/4] = 0x2000; //Clear powerdown
	mxhsadcregs[0x24/4] = 0x31; //Set precharge and SH bypass
	mxhsadcregs[0x30/4] = 0xa; //Set sample num
	mxhsadcregs[0x40/4] = 0x1; //Set seq num
	mxhsadcregs[0x4/4] = 0x40000; //12bit mode

	while(!(mxhsadcregs[0x10/4] & 0x20)) {
		mxhsadcregs[0x50/4]; //Empty FIFO
	}

	mxhsadcregs[0x50/4]; //An extra read is necessary

	mxhsadcregs[0x14/4] = 0xfc000000; //Clr interrupts
	mxhsadcregs[0x4/4] = 0x1; //Set HS_RUN
	usleep(10);
	mxhsadcregs[0x4/4] = 0x08000000; //Start conversion
	while(!(mxhsadcregs[0x10/4] & 0x1)) ; //Wait for interrupt

	for(i = 0; i < 5; i++) {
		x = mxhsadcregs[0x50/4];
		chan[7] += ((x & 0xfff) + ((x >> 16) & 0xfff));
	}


	for(x = 0; x < 7; x++) {
		/* This is where value to voltage conversions would take
		 * place.  Values below are generic and can be used as a 
		 * guideline.
		 *
		 * The math is done to multiply everything up and divide
		 * it down to the resistor network ratio.  It is done 
		 * completely using ints and avoids any floating point math
		 * which is a slower calculation speed.
		 *
		 * All chan[x] values include 10 samples, this needs to be 
		 * divided out to get an average.
		 *
		 * TS-7680
		 *   LRADC channels 3:0
		 *     chan[x] = ((((chan[x]/10)*45177)/1000000)*62);
		 *   LRADC channels 5:4
		 *     bivolt = (((chan[x]/10)*45177)/100000);
                 *     bivolt = (((3*(((435937*bivolt)/1000)-412000))/197));
		 *
		 * Other i.MX28 based SBCs
		 *   LRADC channels 4:1
		 *     chan[x] = ((((chan[x]/10)*45177)/100000)*2);
		 *   LRADC channel 6
		 *     chan[x] = ((((chan[x]/10)*45177)/1000000)*33);
		 *   HSADC
		 *     chan[7] = ((((chan[7]/10)*45177)/100000)*2));
		 */

		printf("LRADC_ADC%d_val=0x%x\n", x, chan[x]/10);
	}
	printf("HSADC_val=0x%x\n", chan[7]/10);

	return 0;
}
