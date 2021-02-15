#!/bin/sh

cp -R /usr/bin/webservice /tmp/
chmod -R 777 /tmp/webservice 

cd /tmp/webservice
./nweb23f 80 /tmp/webservice &

exit 0
