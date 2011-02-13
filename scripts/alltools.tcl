#
# All-Tools TCL, includes toolbox.tcl, toolkit.tcl and moretools.tcl
# toolbox was originally authored by cmwagner <cmwagner@sodre.net>
# toolkit was originally authored by Robey Pointer
# moretools was originally authored by David Sesno <walker@shell.pcrealm.net>
# modified for 1.3.0 bots by TG
#
# Copyright (C) 1999, 2003 - 2011 Eggheads Development Team
#
# Tothwolf  02May1999: rewritten and updated
# guppy     02May1999: updated even more
# Tothwolf  02May1999: fixed what guppy broke and updated again
# Tothwolf  24/25May1999: more changes
# rtc       20Sep1999: added isnumber, changes
# dw        20Sep1999: use regexp for isnumber checking
# Tothwolf  06Oct1999: optimized completely
# krbb      09Jun2000: added missing return to randstring
# Tothwolf  18Jun2000: added ispermowner
# Sup       02Apr2001: added matchbotattr
# Tothwolf  13Jun2001: updated/modified several commands
# Hanno     28Sep2001: fixed testip
# guppy     03Mar2002: optimized
# Souperman 05Nov2002: added ordnumber
# Tothwolf  27Dec2003: added matchbotattrany, optimized ordnumber,
#                      more minor changes
#
# $Id: alltools.tcl,v 1.24 2011/02/13 14:19:33 simple Exp $
#
########################################
#
# Descriptions of available commands:
#
##
## (toolkit):
##
#
# putmsg <nick/chan> <text>
#   send a privmsg to the given nick or channel
#
# putchan <nick/chan> <text>
#   send a privmsg to the given nick or channel
#   (for compat only, this is the same as 'putmsg' above)
#
# putnotc <nick/chan> <text>
#   send a notice to the given nick or channel
#
# putact <nick/chan> <text>
#   send an action to the given nick or channel
#
#
##
## (toolbox):
##
#
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
#   else return empty string
#
# utimerexists <command>
#   if the given command is scheduled by a utimer, return its utimer id
#   else return empty string
#
# inchain <bot>
#   if the given bot is connected to the botnet, return 1
#   else return 0
#   (for compat only, same as 'islinked')
#
# randstring <length>
#   returns a random string of the given length
#
# putdccall <text>
#   send the given text to all dcc users
#
# putdccbut <idx> <text>
#   send the given text to all dcc users except for the given idx
#
# killdccall
#   kill all dcc user connections
#
# killdccbut <idx>
#   kill all dcc user connections except for the given idx
#
#
##
## (moretools):
##
#
# iso <nick> <channel>
#   if the given nick has +o access on the given channel, return 1
#   else return 0
#
# realtime [format]
#   'time' returns the current time in 24 hour format '14:15'
#   'date' returns the current date in the format '21 Dec 1994'
#   not specifying any format will return the current time in
#   12 hour format '1:15 am'
#
# testip <ip>
#   if the given ip is valid, return 1
#   else return 0
#
# number_to_number <number>
#   if the given number is between 1 and 15, return its text representation
#   else return the number given
#
#
##
## (other commands):
##
#
# isnumber <string>
#   if the given string is a valid number, return 1
#   else return 0
#
# ispermowner <handle>
#   if the given handle is a permanent owner, return 1
#   else return 0
#
# matchbotattr <bot> <flags>
#   if the given bot has all the given flags, return 1
#   else return 0
#
# matchbotattrany <bot> <flags>
#   if the given bot has any the given flags, return 1
#   else return 0
#
# ordnumber <string>
#   if the given string is a number, returns the
#   "ordinal" version of that number, i.e. 1 -> "1st",
#   2 -> "2nd", 3 -> "3rd", 4 -> "4th", etc.
#   else return <string>
#
########################################

# So scripts can see if allt is loaded.
set alltools_loaded 1
set allt_version 206

# For backward compatibility.
set toolbox_revision 1007
set toolbox_loaded 1
set toolkit_loaded 1

#
# toolbox:
#

proc putmsg {dest text} {
  puthelp "PRIVMSG $dest :$text"
}

proc putchan {dest text} {
  puthelp "PRIVMSG $dest :$text"
}

proc putnotc {dest text} {
  puthelp "NOTICE $dest :$text"
}

proc putact {dest text} {
  puthelp "PRIVMSG $dest :\001ACTION $text\001"
}

#
# toolkit:
#

proc strlwr {string} {
  string tolower $string
}

proc strupr {string} {
  string toupper $string
}

proc strcmp {string1 string2} {
  string compare $string1 $string2
}

proc stricmp {string1 string2} {
  string compare [string tolower $string1] [string tolower $string2]
}

proc strlen {string} {
  string length $string
}

proc stridx {string index} {
  string index $string $index
}

proc iscommand {command} {
  if {[string compare "" [info commands $command]]} then {
    return 1
  }
  return 0
}

proc timerexists {command} {
  foreach i [timers] {
    if {![string compare $command [lindex $i 1]]} then {
      return [lindex $i 2]
    }
  }
  return
}

proc utimerexists {command} {
  foreach i [utimers] {
    if {![string compare $command [lindex $i 1]]} then {
      return [lindex $i 2]
    }
  }
  return
}

proc inchain {bot} {
  islinked $bot
}

proc randstring {length {chars abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789}} {
  if {([string compare "" $length]) && \
      (![regexp \[^0-9\] $length])} then {
    set count [string length $chars]
    if {$count} then {
      for {set index 0} {$index < $length} {incr index} {
        append result [string index $chars [rand $count]]
      }
    } else {
      error "empty character string"
    }
  } else {
    error "invalid random string length"
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
    if {$j != $idx} then {
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
    if {$j != $idx} then {
      killdcc $j
    }
  }
}

#
# moretools:
#

proc iso {nick chan} {
  matchattr [nick2hand $nick $chan] o|o $chan
}

proc realtime {args} {
  switch -exact -- [lindex $args 0] {
    time {
      return [strftime %H:%M]
    }
    date {
      return [strftime "%d %b %Y"]
    }
    default {
      return [strftime "%I:%M %P"]
    }
  }
}

proc testip {ip} {
  set tmp [split $ip .]
  if {[llength $tmp] != 4} then {
    return 0
  }
  set index 0
  foreach i $tmp {
    if {(([regexp \[^0-9\] $i]) || ([string length $i] > 3) || \
         (($index == 3) && (($i > 254) || ($i < 1))) || \
         (($index <= 2) && (($i > 255) || ($i < 0))))} then {
      return 0
    }
    incr index
  }
  return 1
}

proc number_to_number {number} {
  switch -exact -- $number {
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
    default {
      return $number
    }
  }
}

#
# other commands:
#

proc isnumber {string} {
  if {([string compare "" $string]) && \
      (![regexp \[^0-9\] $string])} then {
    return 1
  }
  return 0
}

proc ispermowner {hand} {
  global owner

  if {([matchattr $hand n]) && \
      ([lsearch -exact [split [string tolower $owner] ", "] \
        [string tolower $hand]] != -1)} then {
    return 1
  }
  return 0
}

proc matchbotattr {bot flags} {
  foreach flag [split $flags ""] {
    if {[lsearch -exact [split [botattr $bot] ""] $flag] == -1} then {
      return 0
    }
  }
  return 1
}

proc matchbotattrany {bot flags} {
  foreach flag [split $flags ""] {
    if {[string first $flag [botattr $bot]] != -1} then {
      return 1
    }
  }
  return 0
}

proc ordnumber {string} {
  if {[isnumber $string]} then {
    set last [string index $string end]
    if {[string index $string [expr [string length $string] - 2]] != 1} then {
      if {$last == 1} then {
        return ${string}st
      } elseif {$last == 2} then {
        return ${string}nd
      } elseif {$last == 3} then {
        return ${string}rd
      }
    }
    return ${string}th
  }
  return $string
}
