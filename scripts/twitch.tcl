package require eggdrop 1.9.0
package require Tcl 8.5

bind raw - 353 twitch:names
bind raw - 366 twitch:eonames
bind out - "% queued" twitch:who

bind rawt - ROOMSTATE   twitch:roomstate
bind rawt - CLEARCHAT   twitch:clearchat
bind rawt - CLEARMSG    twitch:clearmsg
bind rawt - HOSTTARGET  twitch:hosttarget
bind rawt - GLOBALUSERSTATE twitch:guserstate
##### Server doesn't wait for CAP END before registering, can't get GLOBALUSERSTATE?
bind rawt - PRIVMSG     twitch:privmsg
bind rawt - USERNOTICE  twitch:usernotice
bind rawt - WHISPER     twitch:whisper

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
      *rawt:irc:352 $from 352 "$::botnick $chan $nick $nick.tmi.twitch.tv $::server $nick H :0 $chan" ""
   }
   return 0
}

# testuser.tmi.twitch.tv 366 testuser #testchannel :End of /NAMES list
proc twitch:eonames {from key text} {
   lassign [split $text] botnick chan
   # first, make sure the bot knows it's on the channel, fake 352
   *rawt:irc:352 $from 352 "$::botnick $chan $::botnick $::botnick.tmi.twitch.tv $::server $::botnick H :0 $chan" ""
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
  global roomstate
  putlog "=== Detected roomstate values: $tags"
  foreach {k v} $tags {
    if {[dict exists $roomstate $k]} {
      dict set roomstate $k $v
    } else {
      dict append roomstate $k $v
    }
  }
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
  putlog "=== Detected clearmsg values: $tags"
  set clearmsg [makedict $tags]
  putlog "*  Message ID [dict get $clearmsg target-msg-id] sent by [dict get $clearmsg login] was deleted"
}

proc twitch:hosttarget {from key text tags} {
  putlog "=== Detected hosttarget values: $text"
  if [string match [lindex $text 1] ":-"] {
    putlog "* Exited host mode"
  } else {
    if {([llength $text] > 2) && ([lindex $text 2] > 0)} {
      set numviewers "(Viewers: "
      append numviewers [lindex $text 2]
      append numviewers ")"
    } else {
      set numviewers ""
    }
    putlog "* Host mode started for [string trimleft [lindex $text 1] ":"] $numviewers"
  }
}

proc twitch:guserstate {from key text tags} {
  global guserstate
  putlog "=== Detected globaluserstate values: $text"
  set guserstate [makedict $tags]
  putlog "=== The dict is: $guserstate"
}

proc twitch:privmsg {from key text tags} {
  set privtags [makedict $tags]
  putlog "==== $privtags"
  if [dict exists privtags bits] {
    putlog "* [dict get $privtags login-name] cheered [dict get $privtags bits] bits"
  }
}

proc twitch:usernotice {from key text tags} {
  set usertags [makedict $tags]
  if [dict exists usertags display-name] {
    set displayname [dict get $usertags display-name]
  } else {
    set displayname [dict get $usertags login]
  }
### Handle msg-id events sent via usernotice
  if [dict exists usertags msg-id] {
    if [string match [dict get $usertags msg-id] sub] {
      putlog "* $displayname subscribed to the [dict get $usertags msg-param-sub-plan-name] plan"
    }
    if [string match [dict get $usertags msg-id] resub] {
      putlog "* $displayname re-subscribed to the [dict get $usertags msg-param-sub-plan-name] for a total of [dict get $usertags msg-param-cumulative-months]"
    }
    if [string match [dict get $usertags msg-id] subgift] {
      putlog "* $displayname gifted a [dict get $usertags msg-param-sub-plan-name] subscription to [dict get $usertags msg-param-recipient-display-name]"
    }
    if [string match [dict get $usertags msg-id] anonsubgift] {
      putlog "* Someone anonomously gifted a [dict get $usertags msg-param-sub-plan-name] subscription to [dict get $usertags msg-param-recipient-display-name]""
    }
    if [string match [dict get $usertags msg-id] submystergift] {
      putlog "* $displayname sent a mystery gift to [dict get $usertags msg-param-recipient-display-name]"
    }
#### ????????????
    if [string match [dict get $usertags msg-id] giftpaidupgrade] {
      putlog "* [dict get $usertags msg-param-sender-name] gifted a subscription upgrade to $displayname"
    }
#### ????????????
    if [string match [dict get $usertags msg-id] rewardgift] {
      putlog "* $displayname send a reward gift to... someone?"
    }
#### ????????????
    if [string match [dict get $usertags msg-id] anongiftpaidupgrade] {
      putlog "* Someone anonomously gifted a subscription upgrade to $displayname"
    }
    if [string match [dict get $usertags msg-id] raid] {
      putlog "* [dict get $usertags msg-param-displayName] initiated a raid with [dict get $usertags msg-param-viewerCount] users"
    }
#### ????????????
    if [string match [dict get $usertags msg-id] unraid] {
      putlog "* $diplayname ended their raid"
    }
    if [string match [dict get $usertags msg-id] ritual] {
      putlog "* $displayname initiated a [dict get $usertags msg-param-ritual-name] ritual"
    }
    if [string match [dict get $usertags msg-id] bitsbadgetier] {
      putlog "* $displayname earned a [dict get $usertags msg-param-threshold] bits badge"
    }
  }
}

proc twitch:usernotice {from key text tags} {
  putlog "$from whispered: $text"
}
