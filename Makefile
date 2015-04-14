##############################################################################
# Definitions
##############################################################################

# Serial port the Arduino works on. Used when burning HEX code to Arduino's flash memory
SERIAL = /dev/ttyACM0

SCPIMM_VERSION=1.0.0
SCPI_DATE=$(shell date +%F)

ARDUINO_DEF = 105
ARDUINO_DIR = /home/andrey/opt/arduino-1.0.5
ARDUINO_INCLUDES = -I$(ARDUINO_DIR)/hardware/arduino/cores/arduino -I$(ARDUINO_DIR)/hardware/arduino/variants/mega
ARDUINO_CORE_SRC_DIR = $(ARDUINO_DIR)/hardware/arduino/cores/arduino
ARDUINO_CHIP = atmega2560

LIBSCPI_DIR = ../scpi-parser/libscpi
LIBSCPI_SRC_DIR = $(LIBSCPI_DIR)/src
LIBSCPI_INCLUDE_DIR = $(LIBSCPI_DIR)/inc
LIBSCPI_OBJ_DIR = scpi.tmp
LIBSCPI_SRCS = $(addprefix $(LIBSCPI_SRC_DIR)/, \
	debug.c error.c fifo.c ieee488.c \
	minimal.c parser.c units.c utils.c \
	)
LIBSCPI_OBJS = $(addprefix $(LIBSCPI_OBJ_DIR)/, $(notdir $(LIBSCPI_SRCS:.c=.o)))

LIBSCPIMM_DIR = ../scpi-multimeter/libscpimm
LIBSCPIMM_SRC_DIR = $(LIBSCPIMM_DIR)/src
LIBSCPIMM_INCLUDE_DIR = $(LIBSCPIMM_DIR)/inc
LIBSCPIMM_OBJ_DIR = scpimm.tmp
LIBSCPIMM_SRCS = $(addprefix $(LIBSCPIMM_SRC_DIR)/, scpimm.c configure.c display.c dmm.c ieee488.c input.c measure.c sample.c sense.c system.c route.c trigger.c utils.c)
LIBSCPIMM_HEADERS = $(addprefix $(LIBSCPIMM_INCLUDE_DIR)/, scpimm.h)
LIBSCPIMM_OBJS = $(addprefix $(LIBSCPIMM_OBJ_DIR)/, $(notdir $(LIBSCPIMM_SRCS:.c=.o)))

CFLAGS += -g -Os -Wall -fno-exceptions -ffunction-sections -fdata-sections -mmcu=$(ARDUINO_CHIP) -DF_CPU=16000000L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=$(ARDUINO_DEF) -DSCPIMM_NO_CURRENT_DC -DSCPIMM_NO_CURRENT_AC -DSCPIMM_NO_RESISTANCE -DV7_28_REVISION_NO=\"$(SCPIMM_VERSION)\" -DV7_28_REVISION_DATE=\"$(SCPI_DATE)\"

##############################################################################
# Basic targets
##############################################################################

all:	v7-28-arduino.eep v7-28-arduino.hex

v7-28-arduino.o:	src/v7-28-arduino.c src/consts.h src/pins.h
	avr-g++ -c $(CFLAGS) $(ARDUINO_INCLUDES) -I$(LIBSCPI_INCLUDE_DIR) -I$(LIBSCPIMM_DIR)/inc src/v7-28-arduino.c -o v7-28-arduino.o

v7-28-arduino.elf:	v7-28-arduino.o core.a $(LIBSCPI_OBJ_DIR) $(LIBSCPI_OBJS) $(LIBSCPIMM_OBJ_DIR) $(LIBSCPIMM_OBJS)
	avr-gcc -Os -Wl,--gc-sections,--relax -mmcu=$(ARDUINO_CHIP) -o v7-28-arduino.elf v7-28-arduino.o $(LIBSCPI_OBJS) $(LIBSCPIMM_OBJS) core.a -L. -lm

v7-28-arduino.eep:	v7-28-arduino.elf
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 v7-28-arduino.elf v7-28-arduino.eep

v7-28-arduino.hex:	v7-28-arduino.elf
	avr-objcopy -O ihex -R .eeprom v7-28-arduino.elf v7-28-arduino.hex

##############################################################################
# Targets for SCPI library
##############################################################################

$(LIBSCPI_OBJ_DIR):
	mkdir $(LIBSCPI_OBJ_DIR)

$(LIBSCPI_OBJ_DIR)/%.o: $(LIBSCPI_SRC_DIR)/%.c
	avr-g++ -c $(CFLAGS) $(ARDUINO_INCLUDES) -I$(LIBSCPI_INCLUDE_DIR) -o $@ $<

##############################################################################
# Targets for SCPIMM library
##############################################################################

$(LIBSCPIMM_OBJ_DIR):
	mkdir $(LIBSCPIMM_OBJ_DIR)

$(LIBSCPIMM_OBJ_DIR)/%.o: $(LIBSCPIMM_SRC_DIR)/%.c
	avr-g++ -c $(CFLAGS) $(ARDUINO_INCLUDES) -I$(LIBSCPI_INCLUDE_DIR) -I$(LIBSCPIMM_INCLUDE_DIR) -o $@ $<

##############################################################################
# Targets for Arduino code library
##############################################################################

main.cpp.o:	$(ARDUINO_CORE_SRC_DIR)/main.cpp
	avr-g++ -c $(CFLAGS) $(ARDUINO_INCLUDES) $(ARDUINO_CORE_SRC_DIR)/main.cpp -o main.cpp.o
Stream.cpp.o:	$(ARDUINO_CORE_SRC_DIR)/Stream.cpp
	avr-g++ -c $(CFLAGS) $(ARDUINO_INCLUDES) $(ARDUINO_CORE_SRC_DIR)/Stream.cpp -o Stream.cpp.o
HardwareSerial.cpp.o:	$(ARDUINO_CORE_SRC_DIR)/HardwareSerial.cpp
	avr-g++ -c $(CFLAGS) $(ARDUINO_INCLUDES) $(ARDUINO_CORE_SRC_DIR)/HardwareSerial.cpp -o HardwareSerial.cpp.o
Print.cpp.o:	$(ARDUINO_CORE_SRC_DIR)/Print.cpp
	avr-g++ -c $(CFLAGS) $(ARDUINO_INCLUDES) $(ARDUINO_CORE_SRC_DIR)/Print.cpp -o Print.cpp.o
Tone.cpp.o:	$(ARDUINO_CORE_SRC_DIR)/Tone.cpp
	avr-g++ -c $(CFLAGS) $(ARDUINO_INCLUDES) $(ARDUINO_CORE_SRC_DIR)/Tone.cpp -o Tone.cpp.o

wiring.c.o:	$(ARDUINO_CORE_SRC_DIR)/wiring.c
	avr-gcc -c $(CFLAGS) $(ARDUINO_INCLUDES) $(ARDUINO_CORE_SRC_DIR)/wiring.c -o wiring.c.o
wiring_digital.c.o:		$(ARDUINO_CORE_SRC_DIR)/wiring_digital.c
	avr-gcc -c $(CFLAGS) $(ARDUINO_INCLUDES) $(ARDUINO_CORE_SRC_DIR)/wiring_digital.c -o wiring_digital.c.o
WInterrupts.c.o:		$(ARDUINO_CORE_SRC_DIR)/WInterrupts.c
	avr-gcc -c $(CFLAGS) $(ARDUINO_INCLUDES) $(ARDUINO_CORE_SRC_DIR)/WInterrupts.c -o WInterrupts.c.o

core.a: main.cpp.o Stream.cpp.o HardwareSerial.cpp.o Print.cpp.o Tone.cpp.o wiring.c.o wiring_digital.c.o WInterrupts.c.o
	avr-ar rcs core.a main.cpp.o
	avr-ar rcs core.a Stream.cpp.o
	avr-ar rcs core.a HardwareSerial.cpp.o
	avr-ar rcs core.a Print.cpp.o
	avr-ar rcs core.a Tone.cpp.o
	avr-ar rcs core.a wiring.c.o
	avr-ar rcs core.a wiring_digital.c.o
	avr-ar rcs core.a WInterrupts.c.o

##############################################################################
# Disk cleaning targets
##############################################################################

clean-objs:
	rm -rf *.o *.d *.elf *.a *.tmp

clean:	clean-objs
	rm -f *.eep *.hex

dist:
	rm -fr /tmp/v7-28-dist
	mkdir /tmp/v7-28-dist
	cp v7-28-arduino.hex /tmp/v7-28-dist/v7-28-arduino-${SCPIMM_VERSION}.hex
	cd doc; pdflatex v7-28-arduino.tex
	cp doc/v7-28-arduino.pdf /tmp/v7-28-dist/v7-28-arduino-${SCPIMM_VERSION}.pdf
	m4 --define=SCPIMM_VERSION=$(SCPIMM_VERSION) burn.cmd.m4 > /tmp/v7-28-dist/burn.cmd
	cd /tmp/v7-28-dist/; zip -j v7-28-arduino-${SCPIMM_VERSION}.zip *

##############################################################################
# Arduino flash burning targets
##############################################################################

burn:	v7-28-arduino.eep v7-28-arduino.hex
	$(ARDUINO_DIR)/hardware/tools/avrdude -C$(ARDUINO_DIR)/hardware/tools/avrdude.conf -v -v -v -v -p$(ARDUINO_CHIP) -cwiring -P$(SERIAL) -b115200 -D -Uflash:w:./v7-28-arduino.hex:i

