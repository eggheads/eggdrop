# cleanup by Tothwolf 25May1999

# fix for mirc dcc chat /me's
bind filt - "\001ACTION *\001" filt_act
proc filt_act {idx text} {
  return ".me [string trim [join [lrange [split $text] 1 end]] \001]"
}

# fix for telnet session /me's
bind filt - "/me *" filt_telnet_act
proc filt_telnet_act {idx text} {
  return ".me [join [lrange [split $text] 1 end]]"
}
