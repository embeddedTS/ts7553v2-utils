/* SPDX-License-Identifier: BSD-2-Clause */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <cairo/cairo.h>
#include <math.h>
#include <signal.h>
#include <sys/select.h>
#include <fcntl.h>
#include <linux/input.h>

#include "lcd-display.h"

#define INPUT_DEVICE "/dev/input/event1"

const char copyright[] = "Copyright (c) embeddedTS - " __DATE__ " - "
  GITCOMMIT;

static lcdInfo_t *lcd;
static int keypad;
static int get_input_event(void);
static struct input_event event;


static int gotKilled;
static void sigint_handler(int signum) {
   gotKilled = 1;
	signal(signum, sigint_handler);
}

typedef unsigned char arr[16];

static volatile arr *frameBuffer;
static int xRes, yRes;

static inline void setPixel(unsigned int x, unsigned int y) {
   if (x < xRes && y < yRes)
      frameBuffer[y][x/8] |= (1 << (x % 8));
}

static inline void clearPixel(unsigned int x, unsigned int y) {
   if (x < xRes && y < yRes)
      frameBuffer[y][x/8] &= ~(1 << (x % 8));
}


static inline void drawThing(cairo_surface_t *sfc, unsigned int x, unsigned int y) {
   int xx, yy;

   for(xx=0; xx < 20; xx++) {
      for(yy=0; yy < 20; yy++) {
         setPixel(xx+x, yy+y);
      }
   }
   cairo_surface_mark_dirty(sfc);
   cairo_surface_flush(sfc);
}

static inline void undrawThing(cairo_surface_t *sfc, unsigned int x, unsigned int y) { 
   int xx, yy;

   for(xx=0; xx < 20; xx++) {
      for(yy=0; yy < 20; yy++) {
         clearPixel(xx+x, yy+y);
      }
   }
   cairo_surface_mark_dirty(sfc);
   cairo_surface_flush(sfc);
}


int main(void) {

   cairo_t *cr;
   cairo_surface_t *sfc;

   if ((lcd = openDisplay()) == NULL) {
        fprintf(stderr, "ERROR: Can't open display\n");
        return 1;
   }

   if ((keypad = open(INPUT_DEVICE, O_RDONLY)) < 0) {
        fprintf(stderr, "ERROR: Can't open input device '%s'\n", INPUT_DEVICE);
        return 1;
   }

   signal(SIGINT, sigint_handler); // Interrupt, catches Ctrl C
	signal(SIGTERM, sigint_handler); // Terminate

   sfc = cairo_image_surface_create_for_data((unsigned char *)lcd->frameBuffer,
      CAIRO_FORMAT_A1, lcd->displayWidth, lcd->displayHeight, lcd->stride);

   if (sfc) {
      if ((cr = cairo_create(sfc))) {
         int stride, x;
         frameBuffer = (void *)cairo_image_surface_get_data(sfc);
         xRes = cairo_image_surface_get_width(sfc);
         yRes = cairo_image_surface_get_height(sfc);
         stride = cairo_image_surface_get_stride(sfc);

         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
         cairo_set_line_width(cr, 1.0);
         cairo_rectangle(cr, 3, 3, xRes-6, yRes-6);

         cairo_stroke(cr);
         cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);

         cairo_set_font_size(cr, 12.0);
         cairo_move_to(cr, 18, 26);

         cairo_show_text(cr, "Keypad Test");
         cairo_surface_flush(sfc);

         while(get_input_event()) {
            switch (event.type) {
               case EV_KEY:
                  switch(event.code) {
                     case 106: x = 5; break;
                     case 108: x = 37; break;
                     case 103: x = 73; break;
                     case 105: x = 102; break;
                     default: x = 1000;
                  }
                  switch(event.value) {
                     case 1: drawThing(sfc, x, 36);   break;
                     case 0: undrawThing(sfc, x, 36); break;
                  }
            }
         }

         /* Clear the display on the way out... */
         memset(frameBuffer, 0, yRes * stride);
         cairo_surface_mark_dirty (sfc);
         cairo_destroy(cr);
      }
   }

   closeDisplay();
   return 0;
}



static int get_input_event(void) 
{
   int ret;
   fd_set fds;

   FD_ZERO(&fds);
   FD_SET(keypad,&fds);

   while ((ret=select(keypad+1,&fds,NULL,NULL,NULL))) {
      if (gotKilled) return 0;
      if (FD_ISSET(keypad, &fds)) {  
         if (read(keypad,&event,sizeof(event)) == sizeof(event))
            return 1;
      }
  }

  return 0;
}
