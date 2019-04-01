/* SPDX-License-Identifier: BSD-2-Clause */

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>

#include "lcd-display.h"


static lcdInfo_t lcdInfo;
static int fb;
static unsigned long bufferSize;

lcdInfo_t *openDisplay() {
 
   char name[16];
   DIR *dir;
   struct dirent *ent;
   struct fb_fix_screeninfo fixed_info;
   struct fb_var_screeninfo screen_info;
    
   if((dir = opendir("/sys/class/graphics")) == NULL) {
      fprintf(stderr, "ERROR: Cannot access /sys/class/graphics\n");
      return NULL;
   }

   while((ent = readdir(dir)) != NULL) {
      snprintf(name, 16, "/dev/%s", ent->d_name);
      fb = open(name, O_RDWR);
      ioctl(fb, FBIOGET_FSCREENINFO, &fixed_info);      
      ioctl(fb, FBIOGET_VSCREENINFO, &screen_info);

      lcdInfo.displayWidth = screen_info.xres;
      lcdInfo.displayHeight = screen_info.yres;

      if(lcdInfo.displayWidth == 128 && lcdInfo.displayHeight == 64) break;
   }
   if (fb > 0 && ent != NULL) {
      
      ioctl(fb, FBIOGET_VSCREENINFO, &screen_info);

      lcdInfo.bitsPerPixel = screen_info.bits_per_pixel;
      lcdInfo.stride = fixed_info.line_length;
      bufferSize = screen_info.yres_virtual *  lcdInfo.stride;
      
#if (DEBUG)      
      printf("Display: %dx%dx%d stride: %d bufferSize: %d bytes\n", 
         lcdInfo.displayWidth, lcdInfo.displayHeight, lcdInfo.bitsPerPixel, 
         lcdInfo.stride, bufferSize);            
#endif

      lcdInfo.frameBuffer = mmap(NULL, MAX(getpagesize(), bufferSize), 
         PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fb, 0);
      if (lcdInfo.frameBuffer == MAP_FAILED) {
         printf("MAP_FAILED\n");
         close(fb);
         lcdInfo.frameBuffer = NULL;
         return NULL;
      }      
   
      memset((void*)lcdInfo.frameBuffer, 0, bufferSize);      
      return &lcdInfo;
      
    } else {
       fprintf(stderr, "ERROR: Can't open fb device\n");
       return NULL; 
    }
         
}

void closeDisplay(void) {
   
   if (lcdInfo.frameBuffer != NULL)
      munmap((void*)lcdInfo.frameBuffer, bufferSize);
   close(fb);  
}
