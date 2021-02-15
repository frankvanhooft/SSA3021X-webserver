#!/bin/sh

# This installs the webserver files
# Run this from within the USB stick directory. IE, do this:
#   cd /usr/bin/siglent/usr/mass_storage/U-disk0
#   chmod 777 install.sh
#   ./install.sh


# Make the filesystem read-write so we can add the files
mount -o remount,rw /dev/ubi0 /

# Create a webservice folder and copy the files into it
mkdir -p /usr/bin/webservice
cp create_screen_image.sh favicon.ico fb2bmp fb2png fblight index.html nweb23f /usr/bin/webservice/
chmod -R 777 /usr/bin/webservice

# Provide a script to run the webservice on startup
cp S99webservice.sh /etc/rc5.d
chmod 777 /etc/rc5.d/S99webservice.sh

sync
exit 0
