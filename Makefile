all:	src/v7-28-arduino.c src/pins.h
	avr-g++ -c -g -Os -Wall -fno-exceptions -ffunction-sections -fdata-sections -mmcu=atmega2560 -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=105 -I/home/andrey/opt/arduino-1.0.5/hardware/arduino/cores/arduino -I/home/andrey/opt/arduino-1.0.5/hardware/arduino/variants/mega -I/home/andrey/workspace/scpi-parser/libscpi/inc -I/home/andrey/workspace/scpi-multimeter/libscpimm/inc src/v7-28-arduino.c -o v7-28-arduino.o

