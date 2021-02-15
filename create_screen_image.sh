#! /bin/sh

# Simple script to build the specified PNG or BMP file.

# Because the webserver which calls this script forks its processes,
# this script can be called more than once at the same time. 
# We know the index.html file calls for screen_dark.png first.
# So we give that build an advantage, by delaying for 1 second the
# other builds, hence screen_dark.png can start building first.
if test "$1" != "screen_dark.png"
then
  sleep 1
fi

# If a build is currently happening, wait until it's finished.
# The CPU in the spectrum analyzer is very limited, hence we only want
# to do one build at a time, so we don't wait so long for images.
while test -f fbdump
  do
    sleep 1
  done

# We're OK to build...

# Grab the framebuffer and process it into a PNG / BMP
cp /dev/fb0 fbdump

case "$1" in
  screen_dark.png)
    ./fb2png 1024 600 fbdump screen_dark.png
    ;;
    
  screen_light.png)
    ./fblight 614400 fbdump fbdump_light
    ./fb2png 1024 600 fbdump_light screen_light.png
    rm fbdump_light
    ;;
    
  screen_dark.bmp)  
    ./fb2bmp 1024 600 fbdump screen_dark.bmp
    ;;
    
  screen_light.bmp)
    ./fblight 614400 fbdump fbdump_light
    ./fb2bmp 1024 600 fbdump_light screen_light.bmp
    rm fbdump_light
    ;;

  *)
    echo "Error: Unsupported file. Only screen_dark or screen_light dot png or bmp"
    rm fbdump
    exit 1
    ;;
esac

rm fbdump

exit 0

    