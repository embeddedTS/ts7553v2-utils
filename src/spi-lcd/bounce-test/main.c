/* SPDX-License-Identifier: BSD-2-Clause */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
 

#ifndef MAX
#define MAX(x,y)  ((x)>(y)?x:y)
#endif

int running;
static void sigint_handler(int signum) {
   running=0;  
	signal(signum, sigint_handler);
}

typedef unsigned char arr[16];

static unsigned int xRes, yRes, bitsPerPixel, stride;
static volatile arr *frameBuffer;
static unsigned long bufferSize;

#define DEBUG 0

static inline void setPixel(unsigned int x, unsigned int y) {
   
   if (x < xRes && y < yRes)
      frameBuffer[y][x/8] |= (1 << (x % 8));               
}
 
static inline void clearPixel(unsigned int x, unsigned int y) {
   
   if (x < xRes && y < yRes)
      frameBuffer[y][x/8] &= ~(1 << (x % 8));
}


static inline void drawThing(unsigned int x, unsigned int y) {
 
   int xx, yy;
   
   for(xx=0; xx < 38; xx++) {
      for(yy=0; yy < 22; yy++) {
         setPixel(xx+x, yy+y);
      }      
   }

   for(xx=10; xx < 28; xx++) {      
      for(yy=4; yy < 18; yy++) {                    
         clearPixel(xx+x, yy+y);                   
      }
   }
   
}


static void displayUpdateFunction(void) {
   int x, y, going_right, going_down;
   
   running=1;
   signal(SIGINT, sigint_handler);  // Interrupt, catches Ctrl-C
	signal(SIGTERM, sigint_handler); // Terminate
   	
   going_right = going_down = 1;
   x=y=0;
   while(running) {
      memset(frameBuffer, 0, yRes * stride);
      drawThing(x, y);
      usleep(40000);
      
      if (going_right) {
         x++;
         if (x+38 >= xRes) 
            going_right = 0;                           
      } else {
         x--;
         if (x < 1)
            going_right = 1;            
      }         
      
      if (going_down) {
         y++;
         if (y+22 >= yRes) 
            going_down = 0;                           
      } else {
         y--;
         if (y < 1)
            going_down = 1;            
      }                  
   }
      
   memset(frameBuffer, 0, yRes * stride);

}


int main(void) {
   int fb;
   char name[16];
   DIR *dir;
   struct dirent *ent;
   struct fb_var_screeninfo screen_info;
    
   if((dir = opendir("/sys/class/graphics")) == NULL) {
      fprintf(stderr, "ERROR: Cannot access /sys/class/graphics\n");
      return 1;
   }

   while((ent = readdir(dir)) != NULL) {
      snprintf(name, 16, "/dev/%s", ent->d_name);
      fb = open(name, O_RDWR);
      ioctl(fb, FBIOGET_VSCREENINFO, &screen_info);

      xRes = screen_info.xres;
      yRes = screen_info.yres;

      if(xRes == 128 && yRes == 64) break;
   }
   if (fb > 0 && ent != NULL) {
      struct fb_var_screeninfo screen_info;
      struct fb_fix_screeninfo fixed_info;
      
      ioctl(fb, FBIOGET_FSCREENINFO, &fixed_info);      
      ioctl(fb, FBIOGET_VSCREENINFO, &screen_info);

      xRes = screen_info.xres;
      yRes = screen_info.yres;
      bitsPerPixel = screen_info.bits_per_pixel;
      stride = fixed_info.line_length;
      bufferSize = screen_info.yres_virtual *  stride;
      
#if (DEBUG)      
      printf("Display: %dx%dx%d stride: %d bufferSize: %d bytes\n", 
         xRes, yRes, bitsPerPixel, stride, (int)bufferSize);            
#endif

      frameBuffer = mmap(NULL, MAX(getpagesize(), bufferSize), 
         PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fb, 0);
      if (frameBuffer == MAP_FAILED) {
         printf("MAP_FAILED\n");
         close(fb);
         exit(1);
      }
      displayUpdateFunction();
      close(fb);      
   } else {
      fprintf(stderr, "ERROR: Can't open fb device\n");
      return 1;
   }
                  
   return 0;

}
