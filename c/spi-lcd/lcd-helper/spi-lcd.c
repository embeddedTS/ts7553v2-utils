#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
 
#include <linux/spi/spidev.h>

#define DEBUG 0

#define SPI_DEVNAME   "/dev/spidev1.0"

typedef enum {
   BACKLIGHT_OFF=0,
   BACKLIGHT_ON
} backlight_t;

typedef enum {
   LCD_RESETN_ACTIVE=0,
   LCD_RESETN_INACTIVE
} lcd_reset_t;

typedef enum {
   DATA_LUN=0,
   CMD_LUN
} cmdData_t;


int gotHUP;
static void sighup_handler(int signum) {
   gotHUP=1;  
	signal(signum, sighup_handler);
}

static volatile unsigned long *pinctrl;

#ifndef MAX
#define MAX(x,y)  ((x)>(y)?x:y)
#endif


#define LCD_WIDTH    128
#define LCD_HEIGHT   64



static int checkHelperIsRunning(void);
static int spi_fd;
static unsigned char data[8][LCD_WIDTH];

static unsigned int xRes, yRes, bitsPerPixel, stride;
static volatile unsigned char*frameBuffer;
static unsigned char*previousFrameBuffer;
static unsigned long bufferSize;

static unsigned char initString[] = {   
   0xe2,    /* reset */
   0xa2,    /* LCD Bias */
   0xa0,    /* ADC Select = normal */
   0xc8,    /* Common mode = reverse (ie, top of display is row 0) */
   0xa6,    /* display normal (ie, pixel on = black) */
   0x40,    /* Start line = 0 */
   0x24, 
   0x81,    /* Electronic volume set */
   0x19, 
   0x2c, 
   0x2e, 
   0x2f,
   0xa4,    /* display all points = OFF (ie, normal) */
   0xaf,     /* display on */
};

static inline void setPixel(unsigned int x, unsigned int y) {
   
   if (x < LCD_WIDTH && y < LCD_HEIGHT)
      data[y/8][x] |= (1 << (y % 8));               
}
 
static inline void clearPixel(unsigned int x, unsigned int y) {
   
   if (x < LCD_WIDTH && y < LCD_HEIGHT)
      data[y/8][x] &= ~(1 << (y % 8));
}

static void lcd_backlight(backlight_t state)
{
   if (state == BACKLIGHT_ON)
      pinctrl[0x724 / 4] = (1 << 21);  // set GPIO 2.21 (#85) 
   else
      pinctrl[0x728 / 4] = (1 << 21);  // clr
}


static void lcd_reset(lcd_reset_t state)
{
   if (state == LCD_RESETN_ACTIVE)
      pinctrl[0x728 / 4] = (1 << 20);  // clr GPIO 2.20 (#84)
   else
      pinctrl[0x724 / 4] = (1 << 20);  // set   
}


static void lcd_data_cmd_select(cmdData_t state)
{
   if (state == DATA_LUN) 
      pinctrl[0x724 / 4] = (1 << 4);   // set GPIO 2.4 (#68)
   else
      pinctrl[0x728 / 4] = (1 << 4);   // clr
}

static void displayUpdateFunction(void) {

#if (DEBUG)
   int n, j;
#endif
   int i, x, y;
   unsigned char c, spi_mode, spi_bits;
   unsigned long spi_speed;
   struct spi_ioc_transfer xfer[2];
   int status;

   signal(SIGHUP, sighup_handler); // send me a SIGHUP and I'll exit!
   
   // output enables...
   pinctrl[0xb24 / 4] = (1 << 20) | (1 << 21) | (1 << 4);
   // pull-up enables...
   pinctrl[0x624 / 4] = (1 << 20) | (1 << 21) | (1 << 4) | 
                        (1 << 5) | (1 << 6) | (1 << 7);
   // drive strength & voltage...
   pinctrl[0x388 / 4] = (3 << 16) | (3 << 20) | (3 << 24);
   pinctrl[0x384 / 4] = (6 << 16) | (6 << 20) | (6 << 24); 

   
   lcd_reset(LCD_RESETN_ACTIVE);
   usleep(50000);
   lcd_reset(LCD_RESETN_INACTIVE);
   usleep(50000);
   lcd_backlight(BACKLIGHT_ON);
   memset((void*)frameBuffer, 0, bufferSize);
      
   *previousFrameBuffer = 1;  // to force initial lcd update 
   
	lcd_data_cmd_select(CMD_LUN);
   spi_mode = SPI_CPHA | SPI_CPOL;
   ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode);
   spi_bits = 8;
   ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
   ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
   spi_speed = 4000000;
   ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);

   memset(xfer, 0, sizeof(xfer));
   xfer[0].delay_usecs = 100;
   xfer[0].speed_hz = spi_speed;
   xfer[0].bits_per_word = spi_bits;
   xfer[0].tx_buf = (unsigned long)initString;
   xfer[0].len = sizeof(initString);
   xfer[1].rx_buf = 0;
	xfer[1].len = 0;
         
   if ((status = ioctl(spi_fd, SPI_IOC_MESSAGE(2), xfer)) < 0) {
      fprintf(stderr, "Error sending init string to LCD\n");      
      return;
   }

   while(! gotHUP) {
      usleep(50000);    // approx 20Hz refresh
            
      if (! memcmp((void*)frameBuffer, previousFrameBuffer, bufferSize))
         continue;
                  
      memcpy(previousFrameBuffer, (void*)frameBuffer, bufferSize);      
                        
      switch(bitsPerPixel) {
      case 1:                  
         {  
            int bit;
            volatile unsigned char *ucp;
            for(y=0; y < yRes; y++) {
               for(x=0; x < xRes; ) {
                  unsigned long offset = (y * stride) + x/8;
                  ucp = &frameBuffer[offset];
                  for(bit=1; bit < 0x100; bit<<=1, x++) {
                     if (x >= xRes) break;
                     
                     if (*ucp & bit)
                        setPixel(x, y);
                     else
                        clearPixel(x,y);                        
                  }
               }
            }
         }         
         break;
         
      default:
         printf("%d bits-per-pixel not handled yet!\n", bitsPerPixel);
         break;
      }

      for(i=0; i < 8; i++) {               
         lcd_data_cmd_select(CMD_LUN);      
         c = i | 0xb0;   // set page
         xfer[0].tx_buf = (unsigned long)&c;
         xfer[0].len = 1;   
         ioctl(spi_fd, SPI_IOC_MESSAGE(2), xfer);      
         c = 0;         // set column to zero
         ioctl(spi_fd, SPI_IOC_MESSAGE(2), xfer);                 
         c = 0x10;
         ioctl(spi_fd, SPI_IOC_MESSAGE(2), xfer);
         lcd_data_cmd_select(DATA_LUN);
         xfer[0].tx_buf = (unsigned long)&data[i];
         xfer[0].len = LCD_WIDTH;
         ioctl(spi_fd, SPI_IOC_MESSAGE(2), xfer);   
      }
   }

   lcd_backlight(BACKLIGHT_OFF);
   lcd_reset(LCD_RESETN_ACTIVE);
}


int main(void) {
   char name[16];
   int fb_fd, fd;
   DIR *dir;
   struct dirent *ent;
   struct fb_var_screeninfo screen_info;
   struct fb_fix_screeninfo fixed_info;

   if (checkHelperIsRunning()) {
      fprintf(stderr, "lcd-helper is running already\n");
      return 1;
   }
        
   fd = open("/dev/mem", O_RDWR|O_SYNC);   
   pinctrl = (unsigned long *)mmap(0, 8192, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x80018000);
 
   if (pinctrl == MAP_FAILED) {
      fprintf(stderr, "ERROR: Cannot mmap regs\n");
      return 1;
   }
     
   if((dir = opendir("/sys/class/graphics")) == NULL) {
      fprintf(stderr, "ERROR: Cannot access /sys/class/graphics\n");
      return 1;
   }

   while((ent = readdir(dir)) != NULL) {
      snprintf(name, 16, "/dev/%s", ent->d_name);
      fb_fd = open(name, O_RDWR);
      ioctl(fb_fd, FBIOGET_FSCREENINFO, &fixed_info);      
      ioctl(fb_fd, FBIOGET_VSCREENINFO, &screen_info);

      xRes = screen_info.xres;
      yRes = screen_info.yres;

      if(xRes == LCD_WIDTH && yRes == LCD_HEIGHT) break;
      close(fb_fd);
   }
        
   if(fb_fd > 0 && ent != NULL) {
      
      bitsPerPixel = screen_info.bits_per_pixel;
      stride = fixed_info.line_length;
      bufferSize = screen_info.yres_virtual *  stride;
      
#if (DEBUG)      
      printf("Display: %dx%dx%d stride: %d bufferSize: %d bytes\n", 
         xRes, yRes, bitsPerPixel, stride, bufferSize);            
#endif

      if ((previousFrameBuffer = malloc(bufferSize)) == NULL) {   
         fprintf(stderr, "Memory allocation error\n");
         close(fb_fd);
         return 1;
      }
      
      frameBuffer = mmap(NULL, MAX(getpagesize(), bufferSize), 
         PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fb_fd, 0);
      if (frameBuffer == MAP_FAILED) {
         fprintf(stderr, "MAP_FAILED\n");
         close(fb_fd);
         exit(1);
      }

      if ((spi_fd = open(SPI_DEVNAME, O_RDWR | O_SYNC)) < 0) {
         fprintf(stderr, "Failed to open %s\n", SPI_DEVNAME);
         close(fb_fd);
         exit(1);
      }

      if (daemon(0,0)) {
         fprintf(stderr, "can't daemonize!\n");
         close(fb_fd);
         close(spi_fd);
         return 1;
      } 
      
      displayUpdateFunction();
      close(fb_fd);      
      close(spi_fd);
      
   } else {
      fprintf(stderr, "ERROR: Can't open fb device\n");
      return 1;
   }

   return 0;
}

static int checkHelperIsRunning(void) {
	int ret;
	struct dirent entry, *ep;
	DIR *dp;
	int retVal = 0;
	
	if ((dp = opendir("/proc"))) {
		for (ret = readdir_r(dp, &entry, &ep); retVal == 0 && ret == 0 && ep
				!= NULL; ret = readdir_r(dp, &entry, &ep)) {
			if (ep->d_type == DT_DIR) {
				int pidDir = 1;
				char *cp = ep->d_name;

				while (*cp) {
					if (!isdigit(*cp)) {
						pidDir = 0;
						break;
					}
					cp++;
				}
				if (pidDir) {
					char buf[80];
					FILE *f;
					sprintf(buf, "/proc/%s/cmdline", ep->d_name);
					if ((f = fopen(buf, "r"))) {
						if (fgets(buf, sizeof(buf), f)) {
							if (strstr(buf, "lcd-helper")) {
							   if (atoi(ep->d_name) != getpid())
								retVal = 1;
							}
						}
						fclose(f);
					}
				}
			}
		}
		closedir(dp);
	}

	return retVal;
}

