# compat.tcl
#   This script just quickly maps old Tcl functions to the new ones,
#   use this is you are to lazy to get of your butt and update your scripts :D
#   by the way it binds some old command to the new ones
#
# Copyright (C) 2002, 2003 Eggheads Development Team
#
# Wiktor    31Mar2000: added binds and chnick proc
# Tothwolf  25May1999: cleanup
# Tothwolf  06Oct1999: optimized
# rtc       10Oct1999: added [set|get][dn|up]loads functions
#
# $Id: compat.tcl,v 1.8 2002/12/24 02:30:04 wcc Exp $

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

proc time {} {
  strftime "%H:%M"
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

# as you can see it takes a lot of effort to simulate all the old commands
# and adapting your scripts will take such an effort you better include
# this file forever and a day :D

# the following section is for all those old guys who are since '96 in touch
# and can't get used to new command names which make things more logical for newbies

bind dcc - nick *dcc:handle
bind dcc t chnick *dcc:chhandle
