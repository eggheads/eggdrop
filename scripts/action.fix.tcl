# action.fix.tcl
#
# Tothwolf  25May1999: cleanup
# Tothwolf  04Oct1999: changed proc names slightly
# poptix    07Dec2001: handle irssi (and some others) "correct" messages for DCC CTCP
#
# $Id: action.fix.tcl,v 1.5 2001/12/07 18:20:30 poptix Exp $

# fix for mIRC dcc chat /me's
bind filt - "\001ACTION *\001" filt:dcc_action
bind filt - "CTCP_MESSAGE \001ACTION *\001" filt:dcc_action2
proc filt:dcc_action {idx text} {
  return ".me [string trim [join [lrange [split $text] 1 end]] \001]"
}

proc filt:dcc_action2 {idx text} {
  return ".me [string trim [join [lrange [split $text] 2 end]] \001]"
}

# fix for telnet session /me's
bind filt - "/me *" filt:telnet_action
proc filt:telnet_action {idx text} {
  return ".me [join [lrange [split $text] 1 end]]"
}
