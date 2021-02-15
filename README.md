# SSA3021X-webserver
Siglent SSA3021X Spectrum Analyzer webserver

I have one of these spectrum analyzers, and it's excellent. The only thing I miss about it is not having an inbuilt webserver to do a screencapture. By default you use a USB stick, which works fine but can be a bit slow if you need to do a lot of screen captures.

Since the 3021X was released, now there's a 3021X Plus model. That model does contain a webserver, and a touchscreen, and a bunch of other goodies. If I was buying a spectrum analyzer today, I'd buy the Plus version. It's almost the same price as the "non plus" so it's even better value. But given the unit I have, the goal was to add a web screen capture, which is what this project implements.

# Telnet Access
The folks over at https://www.eevblog.com/forum/testgear/hack-of-sigllent-spectrum-analyzer-ssa3021x/ have managed to get a telnet connection. Read those posts on how to do that. 

# Simple Install
Read the file: AA Read Me First.txt for details; with a telnet connection it's pretty simple.

# C Files and Webserver Operation
The webserver nweb23f is a slightly modified version of nweb, originally written by IBM and now found at https://github.com/ankushagarwal/nweb Two changes were made. (1) Logging was edited so that only the most recent log item is in the log - the log file can never become large. (2) The webserver checks if a file named screen_dark or screen_light dot png or bmp is requested. If one of those 4 filenames is requested it calls a shell script named create_screen_image.sh to create the requested file. Those are the only edits. 

create_screen_image.sh calls the following 3 compiled C programs: fb2bmp, fb2png, and fblight.

fb2bmp is framebuffer to BMP. The framebuffer format is RGB565. This C program simply converts the RGB565 to RGB888 and puts the appropriate headers in the file, to create a BMP file. It's simple, mostly a bunch of bit manipulation and some headers. But the downside is a 1024 x 600 image in RGB888 format becomes a 1.8 MB BMP file, which is not ideal when you're copy&pasting a bunch of those images into a document.

fb2png is framebuffer to PNG. This uses lodepng https://github.com/lvandeve/lodepng to create a PNG file. Hence it's slightly more complex than fb2bmp. However the resulting images are around 20k and they look fantastic, so it's well worthwhile. Because there are a limited number of colors used by the SSA3021X, the program first scans through the framebuffer file to find all the unique colors. It then creates a palette. An 8 bit palette allows 256 colors, however in practice the SSA3021X only generates approximately 100 colors so this is very comfortable. Once it has a palette it converts the framebuffer from the original "image of RGB565 colors" into an "image of palette entries", and then passes this to lodepng.

fblight was inspired by what Siglent does with the USB stick screen saving. The spectrum analyzer screen is dark with light text and traces. For printing in a document the reverse is preferred - having a light background with dark text and traces. Siglent does this when saving to a USB stick. fblight does something similar. It scans through each pixel in the framebuffer. "Shades of gray", which includes whites and blacks, are converted to their opposite - darks become light and vice-versa. Yellows are also checked for and made darker. The result is a light background image which is nice to copy into a document.

# Scripts
create_screen_image.sh was described above. Depending upon which of the 4 images is requested, it copies the framebuffer, then runs one or more of the above programs on it to create the requested image.

S99webservice.sh is placed in the /etc/rc5.d directory to permit the webserver to automatically run at startup. It copies the various webserver files from their read-only location in /usr/bin/webservice to a read-write directory in /tmp, then runs the webserver.

install.sh is a fairly trivial script which copies the various files into their correct locations.

# Building the C files
These files need to be compiled for the TI AM335x processor. Hence a suitable build environment must be created. The latest AM335X SDK can be downloaded from https://software-dl.ti.com/processor-sdk-linux/esd/AM335X/latest/index_FDS.html The TI website has instructions on installing and running the tools. Follow them very carefully. Once you're able to compile a simple "hello world" application as they detail, then you're able to compile these C files. There is no make file; just compile each straight from the command line. For example: gcc nweb23f.c -o nweb23f






