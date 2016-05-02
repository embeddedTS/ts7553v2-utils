#ifndef __LCD_DISPLAY_H_8de4ca50_140a_40a1_aa0c_ad8efcd53762_INCLUDED__
#define __LCD_DISPLAY_H_8de4ca50_140a_40a1_aa0c_ad8efcd53762_INCLUDED__


typedef struct {
   unsigned int displayWidth, 
      displayHeight, 
      bitsPerPixel, 
      stride;
      volatile unsigned char*frameBuffer;
   
} lcdInfo_t;

extern lcdInfo_t *openDisplay();
extern void closeDisplay(void) ;

#ifndef MAX
#define MAX(x,y)  ((x)>(y)?x:y)
#endif

#endif
