@ECHO OFF

SET AVR_DIR=C:/Program Files/Arduino/hardware/tools/avr
SET HEX_FILE_NAME=v7-28-arduino-SCPIMM_VERSION.hex

IF "%1" == "" GOTO L1
SET COM=%1
SHIFT

IF "%~1" == "" GOTO L2
SET AVR_DIR=%~1
SHIFT

:L2
IF "%1" == "" GOTO L3
SET HEX_FILE_NAME=%1

:L3
"%AVR_DIR%/bin/avrdude" "-C%AVR_DIR%/etc/avrdude.conf" -v -v -v -v -patmega2560 -cwiring -P\\.\%COM% -b115200 -D -Uflash:w:%HEX_FILE_NAME%:i
EXIT

:L1
@ECHO COM is not specified
GOTO USAGE

:USAGE
@ECHO USAGE: burn.cmd {COM} [{Path to AVR directory}] [{HEX file name}]
@ECHO EXAMPLE: burn.cmd COM12 "C:/Program Files/Arduino/hardware/tools/avr" v7-28-arduino-1.0.0.hex 
