# All-Tools TCL, includes toolbox.tcl, toolkit.tcl and moretools.tcl
# toolbox is Authored by cmwagner@sodre.net
# toolkit is Authored by (Someone claim this)[unknown]
# moretools is Authored by David Sesno(walker@shell.pcrealm.net)
# modified for 1.3.0 bots by TG
# rewritten and updated by Tothwolf 02May1999
# updated even more by guppy 02May1999
# fixed what guppy broke and updated again by Tothwolf 02May1999
# more changes from Tothwolf 24/25May1999
# reversed some of these weird changes and more fixes by rtc 20Sep1999
# version for 1.3 bots only by rtc 24Sep1999

########################################
# Descriptions of avaliable commands:
## (removed from toolkit):
# newflag <flag> - REMOVED numeric flags are no longer supported in this way
#
## (toolkit):
# putmsg <nick> <text>
#   send a message to someone on irc
#
# putchan <channel> <text>
#   send a public message to a channel
#   technically identical to putmsg
#
# putnotc <nick/channel> <text>
#   send a notice to a nick/channel
#
# putact <nick/channel> <text>
#   send an action to a nick/channel
#
## (toolbox):
# strlwr <string>
#   string tolower
#
# strupr <string>
#   string toupper
#
# strcmp <string1> <string2>
#   string compare
#
# stricmp <string1> <string2>
#   string compare (case insensitive)
#
# strlen <string>
#   string length
#
# stridx <string> <index>
#   string index
#
# iscommand <command>
#   if the given command exists, return 1
#   else return 0
#
# timerexists <command>
#   if the given command is scheduled by a timer, return its timer id
#   else return ""
#
# utimerexists <command>
#   if the given command is scheduled by a utimer, return its utimer id
#   else return ""
#
# inchain <bot>
#   if the given bot is connected to the botnet, return 1
#   else return 0
#
# randstring <length>
#   returns a random string of the given length
#
# putdccall <text>
#   send the given text to all dcc users
#   returns nothing
#
# putdccbut <idx> <text>
#   send the given text to all dcc users except for the given idx
#   returns nothing
#
# killdccall
#   kill all dcc user connections
#   returns nothing
#
# killdccbut <idx>
#   kill all dcc user connections except for the given idx
#   returns nothing
#
## (moretools):
# iso <nick> <channel>
#   if the given nick has +o access on the given channel, return 1
#   else return 0
#
# realtime [format]
#   'time' returns the current time in 24 hour format '14:15'
#   'date' returns the current date in the format '21 Dec 1994'
#   not specifying any format will return the current time
#   in 12 hour format '1:15 am'
#
# testip <ip>
#   if the given ip is valid, return 1
#   else return 0
#
# number_to_number <number>
#   if the given number is between 1 and 15, return its analog representation
#   else it returns what it gets.
#
## other
# isnumber <string>
#   returns 1 if 'string' is a number, 0 if not
#
########################################

# So scripts can see if allt is loaded.
set alltools_loaded 1
set allt_version 205

# For backward comptibility.
set toolbox_revision 1007
set toolbox_loaded 1
set toolkit_loaded 1

# Procs.............
proc number_to_number {number} {
  switch -- $number {
    "0" {return "Zero"}
    "1" {return "One"}
    "2" {return "Two"}
    "3" {return "Three"}
    "4" {return "Four"}
    "5" {return "Five"}
    "6" {return "Six"}
    "7" {return "Seven"}
    "8" {return "Eight"}
    "9" {return "Nine"}
    "10" {return "Ten"}
    "11" {return "Eleven"}
    "12" {return "Twelve"}
    "13" {return "Thirteen"}
    "14" {return "Fourteen"}
    "15" {return "Fifteen"}
    default {return $number}
  }
}

proc isnumber {string} {
  return [expr {![regexp \[^0-9\] $string] && $string != ""}]
}

proc testip {address} {
  set testhost [split $address "."]
  if {[llength $testhost] != 4} {return 0}
  foreach part $testhost {
    # >= 0 is just for undertandability, not really needed.
    if {[string length $part] > 3 || ![isnumber $part] || !($part >= 0 && $part <= 255)} {
      return 0
    }
  }
  return 1
}

proc realtime {{type ""}} {
  switch -- $type {
    time {
      return [clock format [clock seconds] -format "%H:%M"]
    }
    date {
      return [clock format [clock seconds] -format "%d %b %Y"]
    }
    "" {
      return [clock format [clock seconds] -format "%I:%M %P"]
    }
  }
}

proc iso {nick chan} {
  return [matchattr [nick2hand $nick $chan] o|o $chan]
}

proc putmsg {target text} {
  putserv "PRIVMSG $target :$text"
}

proc putnotc {target text} {
  putserv "NOTICE $target :$text"
}

proc putchan {target text} {
  putserv "PRIVMSG $target :$text"
}

proc putact {target text} {
  putserv "PRIVMSG $target :\001ACTION $text\001"
}

proc strlwr {string} {
  return [string tolower $string]
}

proc strupr {string} {
  return [string toupper $string]
}

proc strcmp {string1 string2} {
  return [string compare $string1 $string2]
}

proc stricmp {string1 string2} {
  return [string compare [string tolower $string1] [string tolower $string2]]
}

proc strlen {string} {
  return [string length $string]
}

proc stridx {string index} {
  return [string index $string $index]
}

proc iscommand {command} {
  if {[info commands $command] == $command} {
    return 1
  }
  return 0
}

proc timerexists {command} {
  foreach i [timers] {
    if {[lindex $i 1] == $command} {
      return [lindex $i 2]
    }
  }
  return ""
}

proc utimerexists {command} {
  foreach i [utimers] {
    if {[lindex $i 1] == $command} {
      return [lindex $i 2]
    }
  }
  return ""
}

proc inchain {bot} {
  return [islinked $bot]
}

proc randstring {length} {
  # [rtc] not for tcllib as rand is an eggdrop extension
  set result ""
  set chars abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
  set count [string length $chars]
  for {set i 0} {$i < $length} {incr i} {
    append result [string index $chars [rand $count]]
  }
  return $result
}

proc putdccall {text} {
  foreach i [dcclist CHAT] {
    putdcc [lindex $i 0] $text
  }
}

proc putdccbut {idx text} {
  foreach i [dcclist CHAT] {
    set j [lindex $i 0]
    if {$j != $idx} {
      putdcc $j $text
    }
  }
}

proc killdccall {} {
  foreach i [dcclist CHAT] {
    killdcc [lindex $i 0]
  }
}

proc killdccbut {idx} {
  foreach i [dcclist CHAT] {
    set j [lindex $i 0]
    if {$j != $idx} {
      killdcc $j
    }
  }
}

