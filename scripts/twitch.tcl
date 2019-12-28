package require eggdrop 1.9.0
package require Tcl 8.5

bind raw - 353 twitch:names
bind raw - 366 twitch:eonames
bind out - "% queued" twitch:who

set keep-nick 0

proc twitch:who {queue text status} {
   lassign [split $text] cmd chan
   if {($cmd ne "WHO") && ($cmd ne "WHOIS")} {
     return 0
   } else {
     return 1
   }
   # fake a names reply with the nicknames we remember (due to chanlist not being cleared, 1.6.21 bug)
#   twitch:names $::server 353 "$::botnick = $chan :[join [chanlist $chan]]"
#   twitch:eonames $::server 366 "$::botnick $chan :End of /NAMES list"
#   return 1
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
