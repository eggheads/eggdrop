# action.fix.tcl
#
# Copyright (C) 2002 - 2010 Eggheads Development Team
#
# Tothwolf  25May1999: cleanup
# Tothwolf  04Oct1999: changed proc names slightly
# poptix    07Dec2001: handle irssi (and some others) "correct" messages for DCC CTCP
#
# $Id: action.fix.tcl,v 1.12 2010/01/03 13:27:31 pseudo Exp $

# Fix for mIRC dcc chat /me's:
bind filt - "\001ACTION *\001" filt:dcc_action
bind filt - "CTCP_MESSAGE \001ACTION *\001" filt:dcc_action2
proc filt:dcc_action {idx text} {
  return ".me [string trim [join [lrange [split $text] 1 end]] \001]"
}

proc filt:dcc_action2 {idx text} {
  return ".me [string trim [join [lrange [split $text] 2 end]] \001]"
}

# Fix for telnet session /me's:
bind filt - "/me *" filt:telnet_action
proc filt:telnet_action {idx text} {
  return ".me [join [lrange [split $text] 1 end]]"
}
