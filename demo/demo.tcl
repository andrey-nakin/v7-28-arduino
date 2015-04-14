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

# measure DC voltage
puts $fd "MEAS:VOLT?"
puts "VOLTAGE = [gets $fd]"

close $fd
