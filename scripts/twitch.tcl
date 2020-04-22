# This script does... things TBD.
#
# IMPORTANT: To use this script, you need to install additional Tcl libraries.
# On a Debian/Ubuntu-based system, you can do this by running:
# sudo apt-get install tcllib tcl-tls
#
# Then, configure the section below and load this script by adding:
# source scripts/twitch.tcl
# at the bottom of your Eggdrop's config file.
#
#####################################################################
###############    CONFIGURE THIS SECTION  ##########################

# Set this variable to a string-separated list of channels you want
# Eggdrop to check users statuses for at load. This can be written gooder.
set track_chans {#eggdroptest}

# Uncomment this line if you want ... something.
# bind rawt - PRIVMSG     twitch:privmsg

###### DON'T EDIT DOWN HERE UNLESS YOU KNOW WHAT YOU'RE DOING! ######
#####################################################################
package require eggdrop 1.9.0
package require Tcl 8.5
package require http
package require json

foreach c $track_chans {
  twitch:updateusers $c
}
set keep-nick 0

proc twitch:getwebuser {chan} {
  set handle [::http::geturl http://tmi.twitch.tv/group/user/$chan/chatters]
  set body [::http::data $handle]
  set reply [::json::json2dict $body]
  return $reply
}

proc twitch:updateusers {chan} {
   global tw
   
   putlog "Updating Twitch user statuses for $chan..."
   set webchan [string trimleft $chan "#"]
   set reply [twitch:getwebuser $webchan]
   set tw(moderators) [dict get $reply chatters moderators]
   set tw(staff) [dict get $reply chatters staff]
   set tw(admins) [dict get $reply chatters admins]
   set tw(globalmods) [dict get $reply chatters global_mods]
   set tw(viewers) [dict get $reply chatters viewers]
#   foreach nick $nicks {    
#      # fake WHO reply:
#      *raw:irc:352 $from 352 "$::botnick $chan $nick $nick.tmi.twitch.tv $::server $nick H :0 $chan"
#   }
   return 0
}

proc twitch:privmsg {from key text tags} {
### Still need to figure this part out ###
  set privtags [makedict $tags]
  putlog "==== $privtags"
  if [dict exists privtags bits] {
    putlog "* [dict get $privtags login-name] cheered [dict get $privtags bits] bits"
  }
}

# Take a nick and channel, and return if the nick is a moderator on the channel
# If optinal arg update is 1, update the moderator liste from Twitch web API
# before checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 1 if nick is a mod on the channel, 0 if not.

proc ismod {nick chan {update "0"}} {
  global tw

  if {[string index $chan 0] ne "#"} {
    putlog "Invalid channel"
    return -1
  }
  if {$update} {
    set webchan [string trimleft $chan "#"]
    set reply [twitch:getwebuser $webchan]
    set tw(moderators) [dict get $reply chatters moderators]
  }
  if {$nick in $tw(moderators)} {
    return 1
  }
  return 0
}

# Take a nick and channel, and return if the nick is staff on the channel
# If update is 1, update the staff list from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 1 if nick is staff on the channel, 0 if not.

proc isstaff {nick chan update} {
  global tw

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw(staff) [dict get $reply chatters staff]
  }
  if {$nick in $tw(staff)} {
    return 0
  }
  return 1
}

# Take a nick and channel, and return if the nick is an admin on the channel
# If update is 1, update the admin list from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 1 if nick is an admin on the channel, 0 if not.

proc isadmin {nick chan update} {
  global tw

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw(admins) [dict get $reply chatters admins]
  }
  if {$nick in $tw(admins)} {
    return 0
  }
  return 1
}

# Take a nick and channel, and return if the nick is a global mod on the channel
# If update is 1, update the global moderator list from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 1 if nick is a global mod on the channel, 0 if not.

proc isglobalmod {nick chan update} {
  global tw

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw(globalmods) [dict get $reply chatters global_mods]
  }
  if {$nick in $tw(globalmods)} {
    return 0
  }
  return 1
}

# Take a nick and channel, and return if the nick is a viewer on the channel
# If update is 1, update the viewer list from Twitch web API before
# checking. This is advisable if you're running the command once, but you
# probably want to avoid this if you are running it multiple times very quickly.
# Returns 1 if nick is a viewer on the channel, 0 if not.

proc isviewer {nick chan update} {
  global tw

  if (update) {
    set reply [twitch:getwebuser $webchan "chatters"]
    set webchan [string trimleft $chan "#"]
    set tw(viewers) [dict get $reply chatters viewers]
  }
  if {$nick in $tw(viewers)} {
    return 0
  }
  return 1
}
