#!/usr/local/bin/tclsh7.6

if {$argc != 1} {
    error "Syntax: findleak.tcl inputfile"
}

set file [open [lindex $argv 0] r]

set linenum 1
set mem(0,0) 0
set memalloced 0
set memfreed 0

while {[gets $file line]>=0} {
    if {[lindex $line 0] == "Allocating"} {
	set address [lindex $line 7]
	set bytes [lindex $line 1]
	set location [lindex $line 8]

	set mem($address,mem) $bytes
	set mem($address,loc) $location

	set memalloced [expr $memalloced+$bytes]

    } elseif {[lindex $line 0] == "Freeing"} {
	set address [lindex $line 4]
	set memfreed [expr $memfreed+$mem($address,mem)]
	set mem($address,mem) 0

    } else {
	puts "Warning: Parse error in line $linenum"
    }
    set linenum [incr linenum]
}

set names [array names mem]
set total 0
foreach name $names {
    if {[regexp {0x[0-9a-f]+,mem} $name]} {
	set num [scan $name "0x%x,mem" addr]
	set address [format "0x%x" $addr]
	if {$mem($address,mem)} {
	    puts "Leaked $mem($address,mem) bytes at address $address ($mem($address,loc))"
	    set total [expr $total + $mem($address,mem)]
	}
    }
}

puts "================="
puts "Memory allocated: $memalloced Memory freed: $memfreed"
puts "Grand total leak: $total bytes"


	    
