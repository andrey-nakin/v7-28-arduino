#!/bin/sh

ARDUINO_DIR=/usr/share/arduino
HEX_FILE_NAME=v7-28-arduino-SCPIMM_VERSION.hex
ARDUINO_CHIP=atmega2560

if [ "$1" = "" ]
then
	echo "Serial port is not specified"
	echo "Usage: sh burn.sh <serial port> [<path to arduino>] [<path to HEX file>]"
	echo "Example: sh burn.sh /dev/ttyACM0"
	exit
fi

if [ "$2" != "" ]
then
	ARDUINO_DIR=$2
fi

if [ "$3" != "" ]
then
	HEX_FILE_NAME=$3
fi

${ARDUINO_DIR}/hardware/tools/avrdude -C${ARDUINO_DIR}/hardware/tools/avrdude.conf -v -v -v -v -p${ARDUINO_CHIP} -cwiring -P$1 -b115200 -D -Uflash:w:./${HEX_FILE_NAME}:i

