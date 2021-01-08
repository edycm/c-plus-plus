#!/bin/bash

CURRENT_PATH=`cd $(dirname $0)&& pwd`

if [ -d mqtt_broker ] || [ ! -d mqtt_client ]; then
	cd mqtt_broker
	[ ! -d build ] && mkdir build
	[ ! -d bin ] && mkdir bin
	cd build 
	cmake ..
	make 
	cp lib 	$CURRENT_PATH/mqtt_client -a
	cp src/mosquitto ../bin
else
	echo "Directory mqtt_broker not found"	
fi

