# All-Tools TCL, includes toolbox.tcl, toolkit.tcl and moretools.tcl
# toolbox is Authored by cmwagner@sodre.net
# toolkit is Authored by (Someone claim this)[unknown]
# moretools is Authored by David Sesno(walker@shell.pcrealm.net)
# modified for 1.3.0 bots by TG
# rewritten and updated by Tothwolf 02May1999
# updated even more by guppy 02May1999
# fixed what guppy broke and updated again by Tothwolf 02May1999
# more changes from Tothwolf 24/25May1999

########################################
# Descriptions of avaliable commands:
## (toolkit):
# putmsg <nick/chan> <text>
#   send a message to a nick/chan
#
# putchan <nick/chan> <text>
#   identical to putmsg
#
# putnotc <nick/chan> <text>
#   send a notice to a nick/chan
#
# putact <nick/chan> <text>
#   send an action to a nick/chan
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
#   not specifying any format will return the current time with
#   in 12 hour format '1:15 am'
#
# testip <ip>
#   if the given ip is valid, return 1
#   else return 0
#
# number_to_number <number>
#   if the given number is between 1 and 15, return its analog representation
#
########################################

# So scripts can see if allt is loaded.
set alltools_loaded 1
set allt_version 203

# For backward comptibility.
set toolbox_revision 1007
set toolbox_loaded 1
set toolkit_loaded 1

proc putmsg {who text} {
  puthelp "PRIVMSG $who :$text"
}

proc putchan {who text} {
  puthelp "PRIVMSG $who :$text"
}

proc putnotc {who text} {
  puthelp "NOTICE $who :$text"
}

proc putact {who text} {
  puthelp "PRIVMSG $who :\001ACTION $text\001"
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
  if {![string match "" [info commands $command]]} then {
    return 1
  }
  return 0
}

proc timerexists {command} {
  foreach i [timers] {
    if {[string match [lindex $i 1] $command]} then {
      return [lindex $i 2]
    }
  }
  return
}

proc utimerexists {command} {
  foreach i [utimers] {
    if {[string match [lindex $i 1] $command]} then {
      return [lindex $i 2]
    }
  }
  return
}

proc inchain {bot} {
  return [islinked $bot]
}

proc randstring {length} {
  set string ""
  set chars abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
  set count [string length $chars]
  for {set i 0} {[expr $i < $length]} {incr i} {
    append string [string index $chars [rand $count]]
  }
  return $string
}

proc putdccall {text} {
  foreach i [dcclist] {
    set j [lindex $i 0]
    if {[valididx $j]} then {
      putdcc $j $text
    }
  }
  return
}

proc putdccbut {idx text} {
  foreach i [dcclist] {
    set j [lindex $i 0]
    if {([valididx $j]) && (![string match $j $idx])} then {
      putdcc $j $text
    }
  }
  return
}

proc killdccall {} {
  foreach i [dcclist] {
    set j [lindex $i 0]
    if {[valididx $j]} then {
      killdcc $j
    }
  }
  return
}

proc killdccbut {idx} {
  foreach i [dcclist] {
    set j [lindex $i 0]
    if {([valididx $j]) && (![string match $j $idx])} then {
      killdcc $j
    }
  }
  return
}

proc iso {nick chan} {
  return [matchattr [nick2hand $nick $chan] o|o $chan]
}

proc realtime {args} {
  switch -exact [lindex $args 0] {
    time {
      return [strftime "%H:%M"]
    }
    date {
      return [strftime "%d %b %Y"]
    }
    default {
      return [strftime "%l:%M %P"]
    }
  }
}

proc testip {ip} {
  set tmp [split $ip .]
  if {[expr [llength $tmp] != 4]} then {
    return 0
  }
  foreach i [split [join $tmp ""] ""] {
    if {![string match \[0-9\] $i]} then {
      return 0
    }
  }
  set index 0
  foreach i $tmp {
    if {(([expr [string length $i] > 3]) || \
        (([expr $index == 3]) && (([expr $i > 254]) || ([expr $i < 1]))) || \
        (([expr $index <= 2]) && (([expr $i > 255]) || ([expr $i < 0]))))} then {
      return 0
    }
    incr index 1
  }
  return 1
}

proc number_to_number {number} {
  switch -exact $number {
    0 {
      return Zero
    }
    1 {
      return One
    }
    2 {
      return Two
    }
    3 {
      return Three
    }
    4 {
      return Four
    }
    5 {
      return Five
    }
    6 {
      return Six
    }
    7 {
      return Seven
    }
    8 {
      return Eight
    }
    9 {
      return Nine
    }
    10 {
      return Ten
    }
    11 {
      return Eleven
    }
    12 {
      return Twelve
    }
    13 {
      return Thirteen
    }
    14 {
      return Fourteen
    }
    15 {
      return Fifteen
    }
  }
  return
}
