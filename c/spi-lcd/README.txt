lcd-helper/
	Makefile  
	spi-lcd.c	
	
The lcd-helper directory contains the source for the daemon that 
monitors the framebuffer (/dev/fb0) and, if it changes, sends the 
contents to the LCD via SPI.  This program converts the data before
sending it to the LCD.

cairo-test/                          
	Makefile  
	lcd-display.c  
	lcd-display.h  
	main.c

The cairo-text directory contains code for rendering some simple
graphics and text to the framebuffer, using the libcairo graphics
library.  See http://www.cairographics.org for more information.


bounce-test/                         
	Makefile  
	main.c

The bounce-test directory contains a simple program for rendering
directly to the framebuffer.  



Backlight on TS-7553-v2 is on GPIO #85.  This is enabled by the spi-helper
application, but may also be controlled manually...

  echo 85 > /sys/class/gpio/export
  echo high >  /sys/class/gpio/gpio85/direction
    
  
  