package require eggdrop 1.9.0
package require Tcl 8.5

bind raw - 353 twitch:names
bind raw - 366 twitch:eonames
bind out - "% queued" twitch:who

bind rawt - ROOMSTATE   twitch:roomstate
bind rawt - CLEARCHAT   twitch:clearchat
bind rawt - CLEARMSG    twitch:clearmsg
######## TO DO- 
#05:54:12] [@] @login=eggdroptest;room-id=;target-msg-id=34d44768-bd3b-4d4a-bcb6-9f8128219fe8;tmi-sent-ts=1581400452150 :tmi.twitch.tv CLEARMSG #eggdroptest :sdfggs CLEARMSG #eggdroptest :sdfggs
#[05:54:12] triggering bind twitch:clearmsg
#[05:54:12] === Detected clearmsg values: login eggdroptest room-id target-msg-id 34d44768-bd3b-4d4a-bcb6-9f8128219fe8 tmi-sent-ts 1581400452150
#^^^ missing {} for room-id empty var?
bind rawt - HOSTTARGET  twitch:hosttarget

set keep-nick 0

proc twitch:who {queue text status} {
   lassign [split $text] cmd chan
   if {($cmd ne "WHO") && ($cmd ne "WHOIS")} {
     return 0
   } else {
     return 1
   }
   # fake a names reply with the nicknames we remember (due to chanlist not being cleared, 1.6.21 bug)
   twitch:names $::server 353 "$::botnick = $chan :[join [chanlist $chan]]"
   twitch:eonames $::server 366 "$::botnick $chan :End of /NAMES list"
   return 1
}

# testuser.tmi.twitch.tv 353 testuser = #testchannel :testuser2 testuser
proc twitch:names {from key text} {
   set nicks [lassign [split $text] botnick = chan]
   # get rid of the ":"
   set nicks [join $nicks]
   if {[string index $nicks 0] eq ":"} {
      set nicks [string range $nicks 1 end]
   }
   set nicks [split $nicks]
   foreach nick $nicks {
      # fake WHO reply:
      # underworld2.no.quakenet.org 352 TCL #testchannel tcl TCL.users.quakenet.org *.quakenet.org TCL Hx :0 #testchannel
      *raw:irc:352 $from 352 "$::botnick $chan $nick $nick.tmi.twitch.tv $::server $nick H :0 $chan"
   }
   return 0
}

# testuser.tmi.twitch.tv 366 testuser #testchannel :End of /NAMES list
proc twitch:eonames {from key text} {
   lassign [split $text] botnick chan
   # first, make sure the bot knows it's on the channel, fake 352
   *raw:irc:352 $from 352 "$::botnick $chan $::botnick $::botnick.tmi.twitch.tv $::server $::botnick H :0 $chan"
   # fake end of WHO reply
   # underworld2.no.quakenet.org 315 TCL #testchannel:End of /WHO list.
   *raw:irc:315 $from 315 "$::botnick $chan :End of /WHO list."
   return 0
} 

proc makedict {taglist} {
  foreach {k v} $taglist {
    dict set tagdict $k $v
  }
  return $tagdict
}

proc twitch:roomstate {from key text tags} {
  putlog "=== Detected roomstate values: $tags"
  set roomstate [makedict $tags]
  putlog "=== The dict is: $roomstate"
}

proc twitch:clearchat {from key text tags} {
  putlog "=== Detected clearchat values: $tags"
  set clearchat [makedict $tags]
  if [dict exists $clearchat ban-duration] {
    set banlen [dict get $clearchat ban-duration]
  } else {
    set banlen "perm"
  }
  putlog "* [lindex $text 1] was banned from [lindex $text 0] ($banlen)"
}

proc twitch:clearmsg {from key text tags} {
########## BROKE UNTIL TO DO FIXED
  putlog "=== Detected clearmsg values: $tags"
  set clearmsg [makedict $tags]
  putlog "*  Message ID [dict get $clearmsg target-msg-id] sent by [dict get $clearmsg login] was deleted"
}

proc twitch:hosttarget {from key text tags} {
  putlog "=== Detected hosttarget values: $text"
    if ([string match [lindex $text 1] ":-"]) {
      putlog "* Exited host mode"
    } else {
    if ([llength $text] > 2) {
      set numviewers "(Viewers: "
      append numviewers [lindex $text 2]
      append numviewers ")"
    } else {
      set numviewers ""
    }
    putlog "* Host mode started for [lindex $text 1] $numviewers"
  }
}
