/* SPDX-License-Identifier: BSD-2-Clause */

#include <stdio.h>
#include <cairo/cairo.h>
#include <math.h>


#include "lcd-display.h"

static lcdInfo_t *lcd;

int main(void) {
         
   cairo_t *cr;
   cairo_surface_t *sfc;
   
   if ((lcd = openDisplay()) == NULL) {
        fprintf(stderr, "ERROR: Can't open display\n");
        return 1;
   }

   sfc = cairo_image_surface_create_for_data((unsigned char *)lcd->frameBuffer, 
      CAIRO_FORMAT_A1, lcd->displayWidth, lcd->displayHeight, lcd->stride);
        
   if (sfc) {                        
      if ((cr = cairo_create(sfc))) {
         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
         cairo_set_line_width(cr, 1.0);
         cairo_rectangle(cr, 3, 3, lcd->displayWidth-6, lcd->displayHeight-6); // a box                
         
         cairo_stroke(cr);      
         cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);

         cairo_set_font_size(cr, 12.0);
         cairo_move_to(cr, 14, 26);

         cairo_show_text(cr, "Hello, World!");     // some text
         
         cairo_move_to(cr, 80, 50);
         cairo_line_to(cr, 100, 40);               // a line
         cairo_stroke(cr);
         
         cairo_arc(cr, 100, 20, 15, 0, 2 * M_PI);  // a circle         
         cairo_stroke(cr);
         
         cairo_surface_flush(sfc);
                     
         cairo_destroy(cr);                   
      }
   }

   closeDisplay();
   
   return 0;
}


