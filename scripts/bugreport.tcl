proc DBS_init {} {
  global version
  control [set idx [connect 209.224.3.12 25]] DBS_control
}

proc DBS_send_data {idx} {
  global version
  putdcc $idx "Subject: $version"
  putdcc $idx ""
  putdcc $idx ""
  set fd [open DEBUG r]
  while {![eof $fd]} {  
    gets $fd data
    putdcc $idx "$data"
  }
  close $fd
  putdcc $idx "."
}

proc DBS_control {idx arg} {
  global version
  if {$arg==""} { 
  # It's dead Jim!
  }
  # yes, i know very well i could use switch, it's just not worth it in
  # this instance!
  set code [lindex $arg 0]
  if {$code==220} {
    putdcc $idx "MAIL FROM: nobody@eggheads.org"
  }
  if {$code==250 && [lrange $arg 2 end]=="Sender ok"} {
    putdcc $idx "RCPT TO: eggdev@ings.com"
  }
  if {$code==250 && [lrange $arg 2 end]=="Recipient ok"} {
    putdcc $idx "DATA"
  }
  if {$code==354} {
    DBS_send_data $idx
  }
  if {[string match [lrange $arg 2 end] "Message accepted for delivery"]} {  
    putlog "DEBUG file submitted, Thanks!"    
    putdcc $idx "QUIT"
    if {[file exists DEBUG.7]} {
      file delete DEBUG.7
    }
    if {[file exists DEBUG.6]} {
      file rename DEBUG DEBUG.7
    }
    if {[file exists DEBUG.5]} {
      file rename DEBUG.5 DEBUG.6
    }
    if {[file exists DEBUG.4]} {
      file rename DEBUG.4 DEBUG.5
    }
    if {[file exists DEBUG.3]} {
      file rename DEBUG.3 DEBUG.4
    }
    if {[file exists DEBUG.2]} {
      file rename DEBUG.2 DEBUG.3
    }
    if {[file exists DEBUG.1]} {
      file rename DEBUG.1 DEBUG.2
    }
    if {[file exists DEBUG.0]} {
      file rename DEBUG.0 DEBUG.1
    }
    file rename DEBUG DEBUG.0
  }
  if {$code==221} {
    # EOF !
  }
}

putlog "Eggdrop DEBUG file mailing utility loading. - Thanks!"

if {[file exists DEBUG] && [file readable DEBUG]} {
# this date has to be reset to a date about 2 months after
# the release of each version, yes, this will screw up a bit
# on systems with bad clocks
  catch { set numversion }
  if {[unixtime] < 936847569} {
    if {[info exists numversion] && ($numversion > 1032900)} {
      putlog "Found a DEBUG file, sending it to the dev team!"
      DBS_init
    } else {
      putlog "This script requires a newer version of eggdrop. (bugreport.tcl)"
    }
  }
}

