#
# KLined.TCL - Version 1.0
# By Ian Kaney - ikaney@uk.defiant.org
#
# $Id: klined.tcl,v 1.2 1999/12/21 17:35:08 fabian Exp $
#
# Even at the best of times, your bot will get k-lined by one operator or
# another on a server you're running your bot on. This script will 'hopefully'
# handle this by removing it from your bot's server list when it detects
# you've been k-lined there. Thus, stopping IRC server admins getting
# rather peeved at the constant connects from your host.
#
# USAGE:
#      The actual handling of removing the server from your server list
#      and writing it to the 'klines' file is handled automatically when
#      your bot receives the k-line signal, but there are some DCC commands
#      that have been added, these are:
#
#      .klines - Lists the 'klines' file showing servers that your bot
#                has registered as being k-lined on.
#     .unkline <server> - Removes the k-line from the server *joke* ;)
#                         Actually, this removes the server from the list
#                         of servers to remove.
#

# Bindings
# ---
bind load - server remove_kservers
bind raw - 465 woah_klined
bind dcc n klines list_kservers
bind dcc n unkline unkline_server

# Variables
# ---
# Change this to suite your tastes - if you can't be bothered, or
# don't know how, leave it.
set kfile "klines"

proc list_kservers {handle idx args} {
global kfile

putcmdlog "#$handle# klines"
set fd [open $kfile r]
set kservers { }

while {![eof $fd]} {
  set tmp [gets $fd]
  if {[eof $fd]} {break}
  set kservers [lappend kservers [string trim $tmp]]
  }
close $fd
if {[llength $kservers] == 0} {
  putdcc $idx "No k-lined servers."
  return 0
  }
putdcc $idx "My k-lined server list:\n"
foreach tmp $kservers {
  putdcc $idx $tmp
  }
}

proc unkline_server {handle idx args} {
global kfile

set kservers {}

set fd [open $kfile r]
set rem [lindex $args 0]

putcmdlog "#$handle# unkline $rem"

while {![eof $fd]} {
  set tmp [gets $fd]
  if {[eof $fd]} {break}
  set kservers [lappend kservers [string trim $tmp]]
}
close $fd

set fd [open $kfile w]
set flag "0"

foreach tmp $kservers {
  if {$tmp == $rem} {
  set flag "1"
  }
  if {$tmp != $rem} {
     puts $fd $tmp
     }
  }
close $fd
if {$flag == "0"} {
  putdcc $idx "Could not find $rem in the k-lined server list."
  }
if {$flag == "1"} {
  putdcc $idx "Removed server $rem from k-lined server list."
  }
}

proc remove_kservers {module} {
global kfile
global server servers

if {[catch {set fd [open $kfile r]}] != 0} {
set fd [open $kfile w]
close $fd
set fd [open $kfile r]
}

while {![eof $fd]} {
  set from [string trim [gets $fd]]
  set name "*$from*"
  if {[eof $fd]} {break}

  for {set j 0} {$j >= 0} {incr j} {
     set x [lsearch $servers $name]
        if {$x >= 0} {
        set servers [lreplace $servers $x $x]
        }
        if {$x < 0} {
        if {$j >= 0} {
        putlog "Removed server: $from"
	}
	break
        }
     }
  }
close $fd
return 1
}

proc woah_klined {from keyword arg} {

global kfile
global server servers

set kservers {}

set fd [open $kfile r]

while {![eof $fd]} {
  set tmp [gets $fd]
  if {[eof $fd]} {break}
  set kservers [lappend kservers [string trim $tmp]]
}
close $fd

set flag "0"

foreach tmp $kservers {
  if {$tmp == $from} {
  set flag "1"
  }
}

if {$flag != "1"} {
set fd [open $kfile a]
puts $fd $from
close $fd
}

set name "*$from*"

for {set j 0} {$j >= 0} {incr j} {
  set x [lsearch $servers $name]
     if {$x >= 0} {
     set servers [lreplace $servers $x $x]
     }
     if {$x <= 0} {
     if {$j >= 0} {
     putlog "Removed server: $from"
     }
     break
     }
  }
  return 1
}

putlog "TCL loaded: k-lined"
remove_kservers server
