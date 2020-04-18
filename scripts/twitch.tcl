package require eggdrop 1.9.0
package require Tcl 8.5
package require http
package require json

bind raw - 353 twitch:names
bind raw - 366 twitch:eonames

bind rawt - PRIVMSG     twitch:privmsg

proc twitch:getwebuser {chan endpoint} {
  set handle [::http::geturl http://tmi.twitch.tv/group/user/$chan/chatters]
  set body [::http::data $handle]
  set reply [::json::json2dict $body]
  return $reply
}

# testuser.tmi.twitch.tv 353 testuser = #testchannel :testuser2 testuser
proc twitch:names {from key text} {
   global tw_moderators
   global tw_staff
   global tw_admins
   global tw_globalmods
   global tw_viewers
   set nicks [lassign [split $text] botnick = chan]
   # get rid of the ":"
   set nicks [join $nicks]
   if {[string index $nicks 0] eq ":"} {
      set nicks [string range $nicks 1 end]
   }
   set nicks [split $nicks]
   set webchan [string trimleft $chan "#"]
   set reply [twitch:getwebuser $webchan "chatters"]
   set tw_moderators [dict get [dict get $reply chatters] moderators]
   set tw_staff [dict get [dict get $reply chatters] staff]
   set tw_admins [dict get [dict get $reply chatters] admins]
   set tw_globalmods [dict get [dict get $reply chatters] global_mods]
   set tw_viewers [dict get [dict get $reply chatters] viewers]
   putlog $reply
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

proc twitch:privmsg {from key text tags} {
  set privtags [makedict $tags]
  putlog "==== $privtags"
  if [dict exists privtags bits] {
    putlog "* [dict get $privtags login-name] cheered [dict get $privtags bits] bits"
  }
}

# Take a nick and channel, and return if the nick is a moderator on the channel
# If update is 1, update the moderator liste from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 0 if nick is a mod on the channel, 1 if not.

proc ismod {nick chan update} {
  global tw_moderators

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw_moderators [dict get [dict get $reply chatters] moderators]
  } 
  if {[lsearch $tw_moderators $nick] >= 0} {
    return 0
  return 1
}

# Take a nick and channel, and return if the nick is staff on the channel
# If update is 1, update the staff list from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 0 if nick is staff on the channel, 1 if not.

proc isstaff {nick chan update} {
  global tw_staff

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw_staff [dict get [dict get $reply chatters] staff]
  }
  if {[lsearch $tw_staff $nick] >= 0} {
    return 0
  return 1
}

# Take a nick and channel, and return if the nick is an admin on the channel
# If update is 1, update the admin list from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 0 if nick is an admin on the channel, 1 if not.

proc isadmin {nick chan update} {
  global tw_admins

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw_admins [dict get [dict get $reply chatters] admins]
  }
  if {[lsearch $tw_admins $nick] >= 0} {
    return 0
  return 1
}

# Take a nick and channel, and return if the nick is a global mod on the channel
# If update is 1, update the global moderator list from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 0 if nick is a global mod on the channel, 1 if not.

proc ismod {nick chan update} {
  global tw_globalmods

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw_globalmods [dict get [dict get $reply chatters] global_mods]
  }
  if {[lsearch $tw_globalmods $nick] >= 0} {
    return 0
  return 1
}

# Take a nick and channel, and return if the nick is a viewer on the channel
# If update is 1, update the viewer list from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 0 if nick is a viewer on the channel, 1 if not.

proc isviewer {nick chan update} {
  global tw_viewers

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw_viewers [dict get [dict get $reply chatters] viewers]
  }
  if {[lsearch $tw_viewers $nick] >= 0} {
    return 0
  return 1
}



