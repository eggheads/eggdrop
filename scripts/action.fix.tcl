# cleanup by Tothwolf 25May1999

# fix for mirc dcc chat /me's
bind filt -|- "\001ACTION *\001" filt_dcc_action
proc filt_dcc_action {idx text} {
  return ".me [string trim [lrange [split $text] 1 end] \001]"
}

# fix for telnet session /me's
bind filt -|- "/me *" filt_telnet_action
proc filt_telnet_action {idx text} {
  return ".me [lrange [split $text] 1 end]"
}
