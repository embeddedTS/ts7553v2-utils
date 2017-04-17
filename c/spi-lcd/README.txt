lcd-helper/
	Makefile  
	spi-lcd.c	
	
The lcd-helper directory contains the source for the daemon that 
monitors the framebuffer (/dev/fb0) and, if it changes, sends the 
contents to the LCD via SPI.  This program converts the data before
sending it to the LCD.  Note that is requires that the driver for the 
ST7565P (see below) must be loaded.  Also requires that the spidev driver
be loaded.

cairo-test/                          
	Makefile  
	lcd-display.c  
	lcd-display.h  
	main.c

The cairo-text directory contains code for rendering some simple
graphics and text to the framebuffer, using the libcairo graphics
library.  See the cross compiling section of the manual for more
information.

bounce-test/                         
	Makefile  
	main.c

The bounce-test directory contains a simple program for rendering
directly to the framebuffer.  


Backlight on TS-7553-V2 is on GPIO #121.  This is enabled by the spi-helper
application, but may also be controlled manually...

  echo 121 > /sys/class/gpio/export
  echo out > /sys/class/gpio/gpio121/direction
  echo 1 >   /sys/class/gpio/gpio121/value # turns backlight on
  echo 0 >   /sys/class/gpio/gpio121/value # turns backlight off
  
The kernel must have the ST7565P driver enabled for lcd-helper to work.
  modprobe ts-st7565p-fb

