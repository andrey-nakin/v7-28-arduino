#   Copyright (c) 2015 Andrey Nakin
#   All Rights Reserved
#
#	This file is part of v7-28-arduino project.
#
#	v7-28-arduino is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	v7-28-arduino is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with v7-28-arduino.  If not, see <http://www.gnu.org/licenses/>.

##############################################################################
# demo.tcl
# Demonstration of v7-28-arduino
# Usage: demo.tcl <serial port>
# Example: demo.tcl \\.\COM12
##############################################################################

# default values
set port COM1
set mode 9600,n,8,1

if { [llength $argv] > 0 } {
	set port [lindex $argv 0]
}

# open a serial port
if {[catch {set fd [open $port r+]} err]} {
	puts stderr $err
	exit
}

# configure serial port
if {[catch {fconfigure $fd -blocking 1 -buffering line -encoding binary -translation binary -mode $mode -timeout 5000} err]} {
	close $fd
	puts stderr $err
	exit
}

# take a pause while Arduino waits for programming
after 1000

# request device ID string
puts $fd "*IDN?"
set idn [gets $fd]
if { $idn == "" } {
    puts stderr "Multimeter does not respond"
    exit    
} 
puts "IDN = $idn"

# switch to remote mode
puts $fd "SYST:REM"

# measure DC voltage
puts $fd "MEAS:VOLT?"
puts "VOLTAGE = [gets $fd]"

close $fd
