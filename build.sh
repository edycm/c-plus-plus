#!/bin/bash

CURRENT_PATH=`cd $(dirname $0)&& pwd`

[ ! -d bin ] && mkdir bin

if [ -d mqtt_broker ] || [ ! -d mqtt_client ]; then
	cd mqtt_broker
	cp conf ../bin -r
	[ ! -d build ] && mkdir build
	cd build 
	cmake ..
	make 
	cp lib 	$CURRENT_PATH/mqtt_client -a
	cp src/mosquitto ${CURRENT_PATH}/bin
	

	cd ${CURRENT_PATH}/mqtt_client
	[ ! -d build ] && mkdir build
	cd build 
	cmake ..
	make
	cp bin/* ${CURRENT_PATH}/bin

else
	echo "Directory mqtt_broker not found"	
fi

