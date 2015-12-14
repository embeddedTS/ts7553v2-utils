#include <stdio.h>
#include <unistd.h>
#include <dirent.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "i2c-dev.h"

int fpga_init(char *path, char adr)
{
	static int fd = -1;

	if(fd != -1)
		return fd;

	if(path == NULL) {
		// Will always be I2C0 on the 7680
		fd = open("/dev/i2c-0", O_RDWR);
	} else {
		
		fd = open(path, O_RDWR);
	}

	if(!adr) adr = 0x28;

	if(fd != -1) {
		if (ioctl(fd, I2C_SLAVE_FORCE, 0x28) < 0) {
			perror("FPGA did not ACK 0x28\n");
			return -1;
		}
	}

	return fd;
}

void fpoke8(int twifd, uint16_t addr, uint8_t value)
{
	uint8_t data[3];
	data[0] = ((addr >> 8) & 0xff);
	data[1] = (addr & 0xff);
	data[2] = value;
	if (write(twifd, data, 3) != 3) {
		perror("I2C Write Failed");
	}
}

uint8_t fpeek8(int twifd, uint16_t addr)
{
	uint8_t data[2];
	data[0] = ((addr >> 8) & 0xff);
	data[1] = (addr & 0xff);
	if (write(twifd, data, 2) != 2) {
		perror("I2C Address set Failed");
	}
	read(twifd, data, 1);

	return data[0];
}
