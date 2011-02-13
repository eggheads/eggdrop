# compat.tcl
#   This script just quickly maps old Tcl commands to the new ones.
#   Use this if you are too lazy to get off your butt and update your scripts :D
#
# Copyright (C) 2002 - 2011 Eggheads Development Team
#
# Wiktor    31Mar2000: added binds and chnick proc
# Tothwolf  25May1999: cleanup
# Tothwolf  06Oct1999: optimized
# rtc       10Oct1999: added [set|get][dn|up]loads functions
# pseudo    04Oct2009: added putdccraw
# Pixelz    08Apr2010: changed [time] to be compatible with Tcl [time]
#
# $Id: compat.tcl,v 1.20 2011/02/13 14:19:33 simple Exp $

proc gethosts {hand} {
  getuser $hand HOSTS
}

proc addhost {hand host} {
  setuser $hand HOSTS $host
}

proc chpass {hand pass} {
  setuser $hand PASS $pass
}


proc chnick {oldnick newnick} { 
  chhandle $oldnick $newnick
}

# setxtra is no longer relevant 

proc getxtra {hand} {
  getuser $hand XTRA
}

proc setinfo {hand info} {
  setuser $hand INFO $info
}

proc getinfo {hand} {
  getuser $hand INFO
}

proc getaddr {hand} {
  getuser $hand BOTADDR
}

proc setaddr {hand addr} {
  setuser $hand BOTADDR $addr
}

proc getdccdir {hand} {
  getuser $hand DCCDIR
}

proc setdccdir {hand dccdir} {
  setuser $hand DCCDIR $dccdir
}

proc getcomment {hand} {
  getuser $hand COMMENT
}

proc setcomment {hand comment} {
  setuser $hand COMMENT $comment
}

proc getemail {hand} {
  getuser $hand XTRA email
}

proc setemail {hand email} {
  setuser $hand XTRA EMAIL $email
}

proc getchanlaston {hand} {
  lindex [getuser $hand LASTON] 1
}

if {![llength [info commands {TCLTIME}]] && [llength [info commands {time}]]} {
  rename time TCLTIME
}

proc time {args} {
  if {([llength $args] != 0) && [llength [info commands {TCLTIME}]]} {
    if {[llength [info commands {uplevel}]]} {
      uplevel 1 TCLTIME $args
    } else {
      eval TCLTIME $args
    }
  } else {
    strftime "%H:%M"
  }
}

proc date {} {
  strftime "%d %b %Y"
}

proc setdnloads {hand {c 0} {k 0}} {
  setuser $hand FSTAT d $c $k
}

proc getdnloads {hand} {
  getuser $hand FSTAT d
}

proc setuploads {hand {c 0} {k 0}} {
  setuser $hand FSTAT u $c $k
}

proc getuploads {hand} {
  getuser $hand FSTAT u
}

proc putdccraw {idx size text} {
  if {!$idx} {
    putloglev o * "Warning! putdccraw is deprecated. Use putnow instead!"
    putnow $text
    return -code ok
  }
  putloglev o * "Warning! putdccraw is deprecated. Use putdcc instead!"
  if {![valididx $idx]} {return -code error "invalid idx"}
  putdcc $idx $text -raw
}

# as you can see it takes a lot of effort to simulate all the old commands
# and adapting your scripts will take such an effort you better include
# this file forever and a day :D

# Following are some TCL global variables that are obsolete now and have been removed
# but are still defined here so not to break older scripts

set strict-servernames 0

