# sentinel.tcl v1.54 (10 April 1999) by slennox <slenny@ozemail.com.au>
# Latest versions can be found at www.ozemail.com.au/~slenny/eggdrop/
#
# Flood protection system for eggdrop 1.3.17 and later, with integrated
# BitchX CTCP simulation. This script is designed to provide strong
# protection for your bot and channels against large floodnets and proxy
# floods.
#
# Features:
# * Channel CTCP flood protection - sets +mi to halt CTCP flooding and
#   prevent further floodbots from joining.
# * Channel avalanche flood protection - sets +mi to halt avalanche
#   flooding and prevent further floodbots from joining.
# * Channel join-part flood protection - sets +mi to halt join-part
#   flooding.
# * Channel nick flood protection - sets +mi to prevent further floodbots
#   from joining.
# * Bogus username detection - kickbans users with bogus usernames, sets
#   +mi if multiple users with bogus usernames join.
# * Compiles a list of flooders during a flood, and sets bans after +mi is
#   set.
# * Full ban list protection - sets +i if the ban list becomes dangerously
#   full.
# * Bot CTCP flood protection - prevents your bot from flooding off IRC if
#   it is CTCP flooded.
# * Bot MSG flood protection - prevents your bot from flooding off IRC if
#   it is flooded with MSG commands.
# * BitchX simulation - accurate simulation of BitchX CTCP replies and away
#   mode to hide your bot.
# * Public commands lc and uc for locking/unlocking channel.
# * DCC command .sentinel displays current settings.
#
# Notes:
# - Please shut down and restart the bot if loading this script for the
#   first time, or if updating from sentinel.tcl v1.45 or earlier. If you
#   don't do this you may get script errors.
# - Make sure no bots are enforcing channel mode -i and/or -m.
# - If you run sentinel.tcl on multiple bots, it's best not to enable the
#   ban feature on all bots. I recommend enabling it on the most reliable
#   bot, and disabling it on the others by setting sl_ban to 0.
# - Bans are added to the bot's internal ban list, and expire after 24
#   hours by default. If you have +dynamicbans set, the bans may be removed
#   from the channel much sooner than this, but the ban will remain in the
#   bot's internal ban list until it expires.
# - Since sentinel doesn't track the nicks of nick flooders, it will not
#   kick them after locking the channel. If you want nick flooders to be
#   kicked, make sure you are using +enforcebans.
# - Some EFnet servers use a thing called 'server flood protection' which
#   can prevent sentinel from reacting to channel CTCP floods if sl_ccflood
#   is too high. Recommended sl_ccflood setting for bots using such servers
#   is 3:10.
# - For greater protection against large channel floods, I recommend you
#   also use a channel limiter script such as chanlimit.tcl.
#
# Credits: BitchX simulation is based on that used in bitchxpack.tcl,
# originally written by DeathHand. Special thanks to Bass, dw, and guppy
# for their help, and \-\iTman for help with testing the script :)
#
# v1.00 - Initial release
# v1.01 - Fixed erroneous ctcp reply, streamlined 'if' stuff
# v1.02 - Did some testing on 1.1.5 and it doesn't work too well, fixed
# some erroneous replies I found after doing quite a bit of testing,
# getting pretty close to 100% accurate BitchX simulation now (if it isn't
# already) :-)
# v1.10 - Added join-part flood protection, rewrote CTCP flood
# protection for improved multichannel support, lots of other things
# v1.11 - Fixed problem with BitchX away simulator loading when BitchX
# simulation is switched off
# v1.12 - Fixed small problem with a binding which would result in 'no such
# element in array' errors if the bot isn't added as +b to its own userfile
# v1.20 - Added mechanism to stop bot's server queue from becoming bogged
# down with kicks/bans during very large floods which can cause a
# significant delay in setting +mi, bot CTCP flood protection no longer
# triggers on channel CTCP floods, added kicks to part queue to improve
# join-part flood protection, improved method for channel auto-unlock after
# flood, improved interoperability of flood protection and full ban list
# protection (channel won't be unlocked if ban list is full), switched
# channel notices to puthelp queue, increased default setting for join-part
# flood protection, other things not worth mentioning
# v1.30 - Can now kick and ban channel flooders detected before +mi was set
# (ban is of *!*@host.domain.com type), added bot MSG flood protection and
# channel NOTICE flood protection, improved reaction to channel floods if
# bot is not opped, fixed bug in sl_flud proc, fixed problem which can
# cause bot to notice channel multiple times on full ban lists, changed
# binding procedure for sl_lockcmds to allow rehash to disable
# v1.31 - Added nick flood protection, channel notice on full ban list
# changed to puthelp queue, small fix to display of ban setting on
# .sentinel DCC command
# v1.32 - Changed some lsearch checks to validchan and removed unnecessary
# check from sl_nkflood proc
# v1.33 - Streamlined script a bit, added option to prevent +f users
# triggering channel notice flood protection, .sentinel DCC command output
# is now easier to read
# v1.40 - Implemented proper avalanche flood protection (replaces notice
# flood protection), added bogus username detection, streamlined and
# improved kick-ban procs, other minor changes
# v1.45 - Fixed problem with msg flood protection triggering when bots
# flood one another with GO msg command, added putcmdlog for .sentinel
# command, channel lock mechanism now supports putquick for 1.3.24i+ bots,
# makes sure certain 1.3.24i+ settings (ban-bogus/kick-fun/ban-fun) are set
# not to interfere with script, added internal ignore feature for bot ctcp
# and msg flood protection (also fixes multiple notes on bot ctcp/msg
# floods), added string tolower to ban hosts, nick flood protection was
# being triggered by antikill nick collide protection on IRCnet (found by
# Ben Dover) - script no longer counts avalanche/ctcp/nick changes for
# opped users, sl_tmrbans proc was missing [string tolower $chan] line
# causing 'no such element in array' error for channel names with upper
# case characters (found by Ben Dover)
# v1.50 - Flood protection mechanisms can now be selectively disabled in
# the settings, created separate locktimes for +i and +m, flood settings
# are now set in number:seconds format, centralised lock mechanisms, merged
# sl_tmrbans and sl_bansfull into single proc, bot now resets itself ready
# for flood detection if +i or +m is removed before locktime expires,
# addition to ban queue timers to improve ban host buffering, BitchX 
# simulator no longer overwrites init-server setting, makes sure ctcp-mode
# is set to 0 for 1.3.25+ bots, added validuser checks for sl_note, removed
# some redundant isop checks, script now uses eggdrop's ignore feature to
# ignore flooders  * due to changes with binds/procs you will need to shut
# down and restart your bot when updating to this version
# v1.52 - Improved kick and ban procs, made some changes to utimers in
# sl_lock, automatic removal of +i/m after a channel flood can be disabled,
# full ban list protection can be disabled, removed redundant sl_tmrbans
# proc
# v1.53 - Cleaned up BitchX stuff a little and fixed a couple of erroneous
# replies, fixed problems with channels containing []{}\?*" characters
# v1.54 - Fixed problem with +i not being removed if sl_bfmaxbans is set to
# 0 (found by upstream)

## Configuration - please set the options below ##

# Flood settings in number:seconds format, 0:0 to disable
# Bot CTCP flood
set sl_bcflood 5:30
# Bot MSG flood
set sl_bmflood 6:30
# Channel CTCP flood
set sl_ccflood 5:20
# Channel avalanche flood
set sl_avflood 6:20
# Channel bogus username join flood
set sl_boflood 4:20
# Channel join-part flood
set sl_jflood 5:20
# Channel nick flood
set sl_nkflood 6:20

# Length of time in minutes to ban channel flooders (setting this to 0 will
# disable kick-bans and ignores on bogus usernames and channel flooders)
set sl_ban 1440

# Set global bans on channel flooders (if this is off, bans are channel-
# specific)
set sl_globalban 0

# Length of time in minutes to ignore bot flooders (setting this to 0 will
# make ignores permanent - not recommended) - if sl_ban is active, channel
# flooders are also added to the ignore list 
set sl_igtime 240

# Number of kicks per line supported by your IRC network (e.g. 4 for most
# (but not all) EFnet and IRCnet servers), if you're not sure about this,
# leave it set to 1
set sl_kick 1

# Length of time in seconds to set channel +i if flooded - if set to 0, +i
# will not be removed automatically, if not set to 0 the lowest valid
# setting is 15
set sl_ilocktime 120

# Length of time in seconds to set channel +m if flooded - if set to 0, +m
# will not be removed automatically, if not set to 0 the lowest valid
# setting is 15
set sl_mlocktime 60

# Number of bans to allow in the channel ban list before setting the
# channel +i, set this to 0 to disable full ban list protection
set sl_bfmaxbans 19

# User to send note to when channel is flooded, bot is flooded, or ban
# list becomes full, set it to "" to disable
set sl_note "slennox"

# Notice to send to channel when locked due to flood, set it to "" to
# disable
set sl_cfnotice "Channel locked temporarily due to flood, sorry for any inconvenience this may cause :-)"

# Notice to send to channel when locked due to full ban list, set it to ""
# to disable
set sl_bfnotice "Channel locked temporarily due to full ban list, sorry for any inconvenience this may cause :-)"

# Enable 'lc' and 'uc' public commands for locking/unlocking channel - 0 to
# disable, 1 to enable, 2 to require user to be opped on the channel to use
# command
set sl_lockcmds 2

# User flag required to use 'lc' and 'uc' public commands
set sl_lockflag o

# Enable BitchX CTCP and AWAY simulation, off by default
set sl_bxsimul 0


# Don't edit anything below unless you know what you're doing

proc sl_ctcp {nick uhost hand dest key arg} {
  set chan [string tolower $dest]

  global botnick sl_ban sl_bflooded sl_bcflood sl_bcqueue sl_ccbanhost sl_ccbannick sl_ccflood sl_ccqueue sl_flooded sl_note
  if {[lindex $sl_ccflood 0] != 0 && [validchan $chan] && ![isop $nick $chan]} {
    if {$nick == $botnick} {return 0}
    if {$sl_ban != 0 && ![matchattr $hand f|f $chan]} {
      set bhost *!*[string tolower [string range $uhost [string first @ $uhost] end]]
      lappend sl_ccbannick($chan) $nick
      lappend sl_ccbanhost($chan) $bhost
      utimer [expr [lindex $sl_ccflood 1] + 6] "sl_ccbanqueue [split $chan]"
    }
    if {$sl_flooded($chan)} {return 1}
    incr sl_ccqueue($chan)
    utimer [lindex $sl_ccflood 1] "sl_ccqueuereset [split $chan]"
    if {$sl_ccqueue($chan) >= [lindex $sl_ccflood 0]} {
      sl_lock $chan "CTCP flood"
      return 1
    }
    if {$sl_bflooded} {return 1}
  } elseif {[lindex $sl_bcflood 0] != 0 && $dest == $botnick} {
    if {$sl_bflooded} {
      sl_ignore $uhost $hand "CTCP flooder"
      return 1
    }
    incr sl_bcqueue
    utimer [lindex $sl_bcflood 1] sl_bcqueuereset
    if {$sl_bcqueue >= [lindex $sl_bcflood 0]} {
      putlog "sentinel: CTCP flood detected on me! Stopped answering CTCPs temporarily."
      set sl_bflooded 1
      utimer [lindex $sl_bcflood 1] "set sl_bflooded 0"
      if {$sl_note != "" && [validuser $sl_note]} {
        sendnote SENTINEL $sl_note "Bot was CTCP flooded."
      }
      return 1
    }
  }

  global sl_bxsimul
  if {!$sl_bxsimul} {return 0}
  global sl_bxonestack
  if {$sl_bxonestack} {return 1}
  set sl_bxonestack 1
  utimer 2 "set sl_bxonestack 0"

  if {$key == "CLIENTINFO"} {
    set bxcmd [string toupper $arg]
    if {$bxcmd == "NONE"} {
      return 0
    } elseif {$bxcmd == ""} {
      set bxcmd "NONE"
    }
    switch $bxcmd {
      NONE { set text "NOTICE $nick :CLIENTINFO SED UTC ACTION DCC CDCC BDCC XDCC VERSION CLIENTINFO USERINFO ERRMSG FINGER TIME PING ECHO INVITE WHOAMI OP OPS UNBAN IDENT XLINK UPTIME  :Use CLIENTINFO <COMMAND> to get more specific information"}
      SED { set text "NOTICE $nick :CLIENTINFO SED contains simple_encrypted_data"}
      UTC  { set text "NOTICE $nick :CLIENTINFO UTC substitutes the local timezone"}
      ACTION { set text "NOTICE $nick :CLIENTINFO ACTION contains action descriptions for atmosphere"}
      DCC { set text "NOTICE $nick :CLIENTINFO DCC requests a direct_client_connection"}
      CDCC { set text "NOTICE $nick :CLIENTINFO CDCC checks cdcc info for you"}
      BDCC { set text "NOTICE $nick :CLIENTINFO BDCC checks cdcc info for you"}
      XDCC { set text "NOTICE $nick :CLIENTINFO XDCC checks cdcc info for you"}
      VERSION { set text "NOTICE $nick :CLIENTINFO VERSION shows client type, version and environment"}
      CLIENTINFO { set text "NOTICE $nick :CLIENTINFO CLIENTINFO gives information about available CTCP commands"}
      USERINFO { set text "NOTICE $nick :CLIENTINFO USERINFO returns user settable information"}
      ERRMSG { set text "NOTICE $nick :CLIENTINFO ERRMSG returns error messages"}
      FINGER { set text "NOTICE $nick :CLIENTINFO FINGER shows real name, login name and idle time of user"}
      TIME { set text "NOTICE $nick :CLIENTINFO TIME tells you the time on the user's host"}
      PING { set text "NOTICE $nick :CLIENTINFO PING returns the arguments it receives"}
      ECHO { set text "NOTICE $nick :CLIENTINFO ECHO returns the arguments it receives"}
      INVITE { set text "NOTICE $nick :CLIENTINFO INVITE invite to channel specified"}
      WHOAMI { set text "NOTICE $nick :CLIENTINFO WHOAMI user list information"}
      OP { set text "NOTICE $nick :CLIENTINFO OP ops the person if on userlist"}
      OPS { set text "NOTICE $nick :CLIENTINFO OPS ops the person if on userlist"}
      UNBAN { set text "NOTICE $nick :CLIENTINFO UNBAN unbans the person from channel"}
      IDENT { set text "NOTICE $nick :CLIENTINFO IDENT change userhost of userlist"}
      XLINK { set text "NOTICE $nick :CLIENTINFO XLINK x-filez rule"}
      UPTIME { set text "NOTICE $nick :CLIENTINFO UPTIME my uptime"}
      default { set text "NOTICE $nick :ERRMSG CLIENTINFO: $arg is not a valid function"}
    }
    putserv $text
    return 1
  }

  if {$key == "VERSION"} {
    global sl_bxversion sl_bxsystem
    putserv "NOTICE $nick :VERSION BitchX-$sl_bxversion by panasync - $sl_bxsystem : Keep it to yourself!"
    return 1
  }

  if {$key == "USERINFO"} {
    putserv "NOTICE $nick :USERINFO "
    return 1
  }

  if {$key == "FINGER"} {
    global realname sl_bxwhoami sl_bxmachine sl_bxjointime
    set idletime [expr [unixtime] - $sl_bxjointime]
    putserv "NOTICE $nick :FINGER $realname ($sl_bxwhoami@$sl_bxmachine) Idle $idletime seconds"
    return 1
  }

  if {$key == "ECHO"} {
    if {[validchan $chan]} {return 1}
    putserv "NOTICE $nick :ECHO [string range $arg 0 59]"
    return 1
  }

  if {$key == "ERRMSG"} {
    if {[validchan $chan]} {return 1}
    putserv "NOTICE $nick :ERRMSG [string range $arg 0 59]"
    return 1
  }
  
  if {$key == "INVITE"} {
    if {[llength $arg] == 0 || [validchan $chan]} {return 1}
    set chan [lindex [split $arg] 0]
    if {[string index $chan 0] == "#" || [string index $chan 0] == "+" || [string index $chan 0] == "&"} {
      if {[validchan $chan]} {
        putserv "NOTICE $nick :BitchX: Access Denied"
      } else {
        putserv "NOTICE $nick :BitchX: I'm not on that channel"
      }
    }
    return 1
  }

  if {$key == "WHOAMI"} {
    if {[validchan $chan]} {return 1}
    putserv "NOTICE $nick :BitchX: Access Denied"
    return 1
  }

  if {$key == "OP" || $key == "OPS"} {
    if {[llength $arg] == 0 || [validchan $chan]} {return 1}
    set chan [lindex [split $arg] 0]
    putserv "NOTICE $nick :BitchX: I'm not on $chan, or I'm not opped"
    return 1
  }

  if {$key == "UNBAN"} {
    if {[llength $arg] == 0 || [validchan $chan]} {return 1}
    set chan [lindex [split $arg] 0]
    if {[validchan $chan]} {
      putserv "NOTICE $nick :BitchX: Access Denied"
    } else {
      putserv "NOTICE $nick :BitchX: I'm not on that channel"
    }
    return 1
  }

  return 0
}

proc sl_bmflood {nick uhost hand text} {
  global sl_bmflood sl_bflooded sl_bmqueue sl_note
  if {[lindex $sl_bmflood 0] == 0} {return 0}
  if {[matchattr $hand b] && [string tolower [lindex [split $text] 0]] == "go"} {return 0}
  if {$sl_bflooded} {
    sl_ignore $uhost $hand "MSG flooder"
    return 1
  }
  incr sl_bmqueue
  utimer [lindex $sl_bmflood 1] sl_bmqueuereset
  if {$sl_bmqueue >= [lindex $sl_bmflood 0]} {
    putlog "sentinel: MSG flood detected on me! Stopped answering MSGs temporarily."
    set sl_bflooded 1
    utimer [lindex $sl_bmflood 1] "set sl_bflooded 0"
    if {$sl_note != "" && [validuser $sl_note]} {
      sendnote SENTINEL $sl_note "Bot was MSG flooded."
    }
  }
}

proc sl_avflood {from keyword arg} {
  global botnick sl_ban sl_avbanhost sl_avbannick sl_avflood sl_avqueue sl_flooded
  if {[lindex $sl_avflood 0] == 0} {return 0}
  set arg [split $arg]
  if {[lindex $arg 0] == $botnick} {return 0}
  set nick [lindex [split $from !] 0]
  if {$nick == $botnick} {return 0}
  if {[string match *.* $nick]} {return 0}
  set chan [string tolower [lindex $arg 0]]
  if {[validchan $chan]} {
    if {![onchan $nick $chan]} {return 0}
    if {[isop $nick $chan]} {return 0}
    set uhost [getchanhost $nick $chan]
    set hand [nick2hand $nick $chan]
    if {![sl_checkaval [lrange $arg 1 end]]} {return 0}
    if {$sl_ban != 0 && $nick != $botnick && ![matchattr $hand f|f $chan]} {
      set bhost *!*[string tolower [string range $uhost [string first @ $uhost] end]]
      lappend sl_avbannick($chan) $nick
      lappend sl_avbanhost($chan) $bhost
      utimer [expr [lindex $sl_avflood 1] + 6] "sl_avbanqueue [split $chan]"
    }
    if {$sl_flooded($chan)} {return 0}
    incr sl_avqueue($chan)
    utimer [lindex $sl_avflood 1] "sl_avqueuereset [split $chan]"
    if {$sl_avqueue($chan) >= [lindex $sl_avflood 0]} {
      sl_lock $chan "AVALANCHE flood"
    }
  }
}

proc sl_checkaval {text} {
  if {[expr [regsub -all -- "\007" $text "" temp] + [regsub -all -- "\001" $temp "" temp]] >= 3} {return 1}
  return 0
}

proc sl_nkflood {nick uhost hand chan newnick} {
  global botnick sl_ban sl_flooded sl_nkbanhost sl_nkflood sl_nkqueue
  if {[lindex $sl_nkflood 0] == 0} {return 0}
  set chan [string tolower $chan]
  if {[isop $newnick $chan]} {return 0}
  if {$sl_ban != 0 && $newnick != $botnick && ![matchattr $hand f|f $chan]} {
    set bhost *!*[string tolower [string range $uhost [string first @ $uhost] end]]
    lappend sl_nkbanhost($chan) $bhost
    utimer [expr [lindex $sl_nkflood 1] + 6] "sl_nkbanqueue [split $chan]"
  }
  if {$sl_flooded($chan)} {return 0}
  incr sl_nkqueue($chan)
  utimer [lindex $sl_nkflood 1] "sl_nkqueuereset [split $chan]"
  if {$sl_nkqueue($chan) >= [lindex $sl_nkflood 0]} {
    sl_lock $chan "NICK flood"
  }
}

proc sl_jflood {nick uhost hand chan} {
  global botnick sl_ban sl_bobanhost sl_bobannick sl_boflood sl_boqueue sl_flooded sl_globalban sl_jbanhost sljbannick sl_jflood sl_jqueue sl_pqueue
  if {$nick == $botnick} {return 0}
  set ihost *!*[string tolower [string range $uhost [string first @ $uhost] end]]
  if {[isignore $ihost]} {
    killignore $ihost
  }
  set chan [string tolower $chan]
  if {[sl_checkbogus [lindex [split $uhost @] 0]]} {
    if {$sl_ban != 0 && ![matchattr $hand f|f $chan]} {
      set bhost *!*[string tolower [string range $uhost [string first @ $uhost] end]]
      if {[botisop $chan] && !$sl_flooded($chan)} {
        putserv "KICK $chan $nick :sentinel: BOGUS username"
        if {$sl_globalban} {
          newban $bhost sentinel "BOGUS username" $sl_ban
        } else {
          newchanban $chan $bhost sentinel "BOGUS username" $sl_ban
        }
        sl_ignore $bhost * "BOGUS username"
      }
      lappend sl_bobannick($chan) $nick
      lappend sl_bobanhost($chan) $bhost
      utimer [lindex $sl_boflood 1] "sl_bobanqueue [split $chan]"
    }
    if {[lindex $sl_boflood 0] != 0 && !$sl_flooded($chan)} {
      incr sl_boqueue($chan)
      utimer [lindex $sl_boflood 1] "sl_boqueuereset [split $chan]"
      if {$sl_boqueue($chan) >= [lindex $sl_boflood 0]} {
        sl_lock $chan "BOGUS joins"
      }
    }
  }
  if {[lindex $sl_jflood 0] == 0} {return 0}
  if {![matchattr $hand f|f $chan]} {
    set bhost *!*[string tolower [string range $uhost [string first @ $uhost] end]]
    lappend sl_jbannick($chan) $nick
    lappend sl_jbanhost($chan) $bhost
    utimer [expr [lindex $sl_jflood 1] + 6] "sl_jbanqueue [split $chan]"
  }
  if {$sl_flooded($chan)} {return 0}
  incr sl_jqueue($chan)
  utimer [lindex $sl_jflood 1] "sl_jqueuereset [split $chan]"
  if {$sl_jqueue($chan) >= [lindex $sl_jflood 0] && $sl_pqueue($chan) >= [lindex $sl_jflood 0]} {
    sl_lock $chan "JOIN-PART flood"
  }
}

proc sl_checkbogus {ident} {
  if {[regsub -all -- "\[^\041-\176\]" $ident "" temp] >=1} {return 1}
  return 0
}

proc sl_pflood {nick uhost hand chan} {
  global botnick sl_ban sl_flooded sl_jflood sl_pbanhost sl_pbannick sl_pqueue
  if {[lindex $sl_jflood 0] == 0} {return 0}
  if {$nick == $botnick} {return 0}
  set chan [string tolower $chan]
  if {$sl_ban != 0 && ![matchattr $hand f|f $chan]} {
    set bhost *!*[string tolower [string range $uhost [string first @ $uhost] end]]
    lappend sl_pbannick($chan) $nick
    lappend sl_pbanhost($chan) $bhost
    utimer [expr [lindex $sl_jflood 1] + 6] "sl_jbanqueue [split $chan]"
  }
  if {$sl_flooded($chan)} {return 0}
  incr sl_pqueue($chan)
  utimer [lindex $sl_jflood 1] "sl_pqueuereset [split $chan]"
}

proc sl_pfloodk {nick uhost hand chan kicked reason} {
  global botnick sl_flooded sl_jflood sl_pqueue
  if {[lindex $sl_jflood 0] == 0} {return 0}
  if {$kicked == $botnick} {return 0}
  set chan [string tolower $chan]
  if {$sl_flooded($chan)} {return 0}
  incr sl_pqueue($chan)
  utimer [lindex $sl_jflood 1] "sl_pqueuereset [split $chan]"
}

proc sl_ccqueuereset {chan} {
  global sl_ccqueue
  incr sl_ccqueue($chan) -1
}

proc sl_bcqueuereset {} {
  global sl_bcqueue
  incr sl_bcqueue -1
}

proc sl_bmqueuereset {} {
  global sl_bmqueue
  incr sl_bmqueue -1
}

proc sl_avqueuereset {chan} {
  global sl_avqueue
  incr sl_avqueue($chan) -1
}

proc sl_nkqueuereset {chan} {
  global sl_nkqueue
  incr sl_nkqueue($chan) -1
}

proc sl_boqueuereset {chan} {
  global sl_boqueue
  incr sl_boqueue($chan) -1
}

proc sl_jqueuereset {chan} {
  global sl_jqueue
  incr sl_jqueue($chan) -1
}

proc sl_pqueuereset {chan} {
  global sl_pqueue
  incr sl_pqueue($chan) -1
}

proc sl_ccbanqueue {chan} {
  global sl_ccbanhost sl_ccbannick
  set sl_ccbannick($chan) [lrange sl_ccbannick($chan) 1 end]
  set sl_ccbanhost($chan) [lrange sl_ccbanhost($chan) 1 end]
}

proc sl_avbanqueue {chan} {
  global sl_avbanhost sl_avbannick
  set sl_avbannick($chan) [lrange sl_avbannick($chan) 1 end]
  set sl_avbanhost($chan) [lrange sl_avbanhost($chan) 1 end]
}

proc sl_nkbanqueue {chan} {
  global sl_nkbanhost
  set sl_nkbanhost($chan) [lrange sl_nkbanhost($chan) 1 end]
}

proc sl_bobanqueue {chan} {
  global sl_bobanhost sl_bobannick
  set sl_bobannick($chan) [lrange sl_bobannick($chan) 1 end]
  set sl_bobanhost($chan) [lrange sl_bobanhost($chan) 1 end]
}

proc sl_jbanqueue {chan} {
  global sl_jbanhost sl_jbannick
  set sl_jbannick($chan) [lrange sl_jbannick($chan) 1 end]
  set sl_jbanhost($chan) [lrange sl_jbanhost($chan) 1 end]
}

proc sl_pbanqueue {chan} {
  global sl_pbanhost sl_pbannick
  set sl_pbannick($chan) [lrange sl_pbannick($chan) 1 end]
  set sl_pbanhost($chan) [lrange sl_pbanhost($chan) 1 end]
}

proc sl_flud {nick uhost hand type chan} {
  global sl_flooded
  set chan [string tolower $chan]
  if {[validchan $chan] && $sl_flooded($chan)} {return 1}
}

proc sl_dokicks {chan} {
  global botnick sl_avbannick sl_bobannick sl_ccbannick sl_jbannick sl_pbannick
  if {![onchan $botnick $chan] || ![botisop $chan]} {return 0}
  sl_kick $chan $sl_ccbannick($chan) "CTCP flooder"
  sl_kick $chan $sl_avbannick($chan) "AVALANCHE flooder"
  sl_kick $chan $sl_bobannick($chan) "BOGUS username"
  set jklist $sl_jbannick($chan)
  set pklist $sl_pbannick($chan)
  if {$jklist != "" && $pklist != ""} {
    set klist ""
    foreach knick $jklist {
      if {[lsearch -exact $pklist $knick] != -1} {
        lappend klist $knick
   	  }
    }
    sl_kick $chan $klist "JOIN-PART flooder"
  }
}

proc sl_setbans {chan} {
  global botnick sl_avbanhost sl_bobanhost sl_ccbanhost sl_nkbanhost sl_jbanhost sl_pbanhost
  if {![onchan $botnick $chan]} {return 0}
  sl_ban $chan $sl_ccbanhost($chan) "CTCP flooder"
  sl_ban $chan $sl_avbanhost($chan) "AVALANCHE flooder"
  sl_ban $chan $sl_nkbanhost($chan) "NICK flooder"
  sl_ban $chan $sl_bobanhost($chan) "BOGUS username"
  set jblist $sl_jbanhost($chan)
  set pblist $sl_pbanhost($chan)
  if {$jblist != "" && $pblist != ""} {
    set blist ""
    foreach bhost $jblist {
      if {[lsearch -exact $pblist $bhost] != -1} {
        lappend blist $bhost
      }
    }
    sl_ban $chan $blist "JOIN-PART flooder"
  }
}

proc sl_kick {chan klist reason} {
  global sl_kick
  if {$klist != ""} {
    set kicklist ""
    foreach knick $klist {
      if {[lsearch -exact $kicklist $knick] == -1} {
        lappend kicklist $knick
      }
    }
    foreach knick $kicklist {
      if {[onchan $knick $chan]} {
        lappend ksend $knick
        if {[llength $ksend] >= $sl_kick} {
          set ksend [join $ksend ","]
          putserv "KICK $chan $ksend :sentinel: $reason"
          unset ksend
        }
      }
    }
    if {[info exists ksend]} {
      set ksend [join $ksend ","]
      putserv "KICK $chan $ksend :sentinel: $reason"
      unset ksend
    }
  }
}

proc sl_ban {chan blist reason} {
  global sl_ban sl_globalban
  if {$blist != ""} {
    set banlist ""
    foreach bhost $blist {
      if {[lsearch -exact $banlist $bhost] == -1} {
        lappend banlist $bhost
      }
    }
    if {$sl_globalban} {
      foreach bhost $banlist {
        if {[isban $bhost] || [ispermban $bhost]} {continue}
        newban $bhost sentinel $reason $sl_ban
        putlog "sentinel: banned $bhost ($reason)"
        sl_ignore $bhost * $reason
      }
    } else {
      foreach bhost $banlist {
        if {[isban $bhost $chan] || [ispermban $bhost $chan]} {continue}
        newchanban $chan $bhost sentinel $reason $sl_ban
        putlog "sentinel: banned $bhost on $chan ($reason)"
        sl_ignore $bhost * $reason
      }
    }
  }
}

proc sl_ignore {uhost hand flood} {
  global sl_igtime
  if {$hand != "*"} {
    foreach chan [string tolower [channels]] {
      if {[matchattr $hand f|f $chan]} {return 0}
    }
  }
  set ihost *!*[string tolower [string range $uhost [string first @ $uhost] end]]
  if {[isignore $ihost]} {return 1}
  newignore $ihost sentinel $flood $sl_igtime
  putlog "sentinel: added $ihost to ignore list ($flood)"
  return 1
}

proc sl_lock {chan flood} {
  global numversion sl_bflooded sl_cfnotice sl_flooded sl_ilocktime sl_mlocktime sl_note
  if {[botisop $chan]} {
    if {$numversion >= 1032400} {
      putquick "MODE $chan +mi"
    } else {
      putserv "MODE $chan +mi"
    }
    set sl_flooded($chan) 1
    set sl_bflooded 1
    foreach utimer [utimers] {
     if {[string match "sl_unlock $chan*" [lindex $utimer 1]] || [string match "set sl_flooded($chan) 0" [lindex $utimer 1]] || [string match "pushmode $chan -m" [lindex $utimer 1]] || [string match "set sl_bflooded 0" [lindex $utimer 1]]} {
       killutimer [lindex $utimer 2]
     }
    }
    if {$sl_ilocktime != 0} {
      utimer $sl_ilocktime "sl_unlock [split $chan] i"
    }
    if {$sl_mlocktime != 0} {
      utimer $sl_mlocktime "sl_unlock [split $chan] m"
    }
    utimer 120 "set sl_flooded([split $chan]) 0"
    utimer 120 "set sl_bflooded 0"
    putlog "sentinel: $flood detected on $chan! Channel locked temporarily."
    if {$sl_cfnotice != ""} {
      puthelp "NOTICE $chan :$sl_cfnotice"
    }
    if {$sl_note != "" && [validuser $sl_note]} {
      sendnote SENTINEL $sl_note "Locked $chan due to $flood."
    }
  } else {
    set sl_flooded($chan) 1
    set sl_bflooded 1
    putlog "sentinel: $flood detected on $chan! Cannot lock channel because I'm not opped."
    utimer 120 "set sl_flooded([split $chan]) 0"
    utimer 120 "set sl_bflooded 0"
  }
}

proc sl_unlock {chan umode} {
  global sl_bfmaxbans sl_flooded
  set sl_flooded($chan) 0
  if {![botisop $chan]} {return 0}
  if {$umode == "i" && [string match *i* [lindex [getchanmode $chan] 0]]} {
    if {$sl_bfmaxbans != 0 && [llength [chanbans $chan]] >= $sl_bfmaxbans} {
      putlog "sentinel: not removing +i on $chan due to full ban list."
    } else {
      pushmode $chan -i
      putlog "sentinel: removed +i on $chan"
    }
  } elseif {$umode == "m" && [string match *m* [lindex [getchanmode $chan] 0]]} {
    pushmode $chan -m
    putlog "sentinel: removed +m on $chan"
  }
}

proc sl_lc {nick uhost hand chan arg} {
  global numversion sl_lockcmds
  set chan [string tolower $chan]
  if {![botisop $chan]} {return 0}
  if {$sl_lockcmds == 2 && ![isop $nick $chan]} {return 0}
  if {$numversion >= 1032400} {
    putquick "MODE $chan +mi"
  } else {
    putserv "MODE $chan +mi"
  }
  putcmdlog "sentinel: channel lock requested by $hand on $chan"
}

proc sl_uc {nick uhost hand chan arg} {
  global sl_lockcmds
  set chan [string tolower $chan]
  if {![botisop $chan]} {return 0}
  if {$sl_lockcmds == 2 && ![isop $nick $chan]} {return 0}
  putserv "MODE $chan -mi"
  putcmdlog "sentinel: channel unlock requested by $hand on $chan"
}

proc sl_mode {nick uhost hand chan mode victim} {
  global sl_ban sl_bfmaxbans sl_bfnotice sl_bfull sl_flooded sl_note
  set mode [lindex [split $mode] 0]
  set chan [string tolower $chan]
  if {$mode == "+b" && $sl_bfmaxbans != 0 && !$sl_bfull($chan) && ![string match *i* [lindex [getchanmode $chan] 0]] && [botisop $chan] && [llength [chanbans $chan]] >= $sl_bfmaxbans} {
    putserv "MODE $chan +i"
    set sl_bfull($chan) 1
    utimer 5 "set sl_bfull([split $chan]) 0"
    putlog "sentinel: locked $chan due to full ban list!"
    if {$sl_bfnotice != ""} {
      puthelp "NOTICE $chan :$sl_bfnotice"
    }    
    if {$sl_note != ""} {
      sendnote SENTINEL $sl_note "Locked $chan due to full ban list."
    }
  } elseif {$sl_ban && $mode == "+i" && $sl_flooded($chan)} {
    utimer 3 "sl_dokicks [split $chan]"
    utimer 6 "sl_setbans [split $chan]"
  } elseif {$mode == "-i" || $mode == "-m"} {
    if {$sl_flooded($chan)} {
      set sl_flooded($chan) 0
      foreach utimer [utimers] {
        if {[string match "sl_unlock $chan*" [lindex $utimer 1]]} {
          killutimer [lindex $utimer 2]
        }
      }
    }
  }
}

proc sl_dcc {hand idx arg} {
  global sl_avflood sl_ban sl_bcflood sl_boflood sl_bmflood sl_bxsimul sl_bfmaxbans sl_ccflood sl_globalban sl_igtime sl_jflood sl_kick sl_lockcmds sl_lockflag sl_ilocktime sl_mlocktime sl_nkflood sl_note sl_ver
  putcmdlog "#$hand# sentinel"
  putidx $idx "This bot is protected by sentinel.tcl $sl_ver by slennox"
  putidx $idx "Latest versions: www.ozemail.com.au/~slenny/eggdrop/"
  putidx $idx "Current settings"
  if {$sl_bcflood == 0} {
    putidx $idx "- Bot CTCP flood:           Off"
  } else {
    putidx $idx "- Bot CTCP flood:           [lindex $sl_bcflood 0] in [lindex $sl_bcflood 1] secs"
  }
  if {$sl_bmflood == 0} {
    putidx $idx "- Bot MSG flood:            Off"
  } else {
    putidx $idx "- Bot MSG flood:            [lindex $sl_bmflood 0] in [lindex $sl_bmflood 1] secs"
  }
  if {$sl_ccflood == 0} {
    putidx $idx "- Channel CTCP flood:       Off"
  } else {
    putidx $idx "- Channel CTCP flood:       [lindex $sl_ccflood 0] in [lindex $sl_ccflood 1] secs"
  }
  if {$sl_avflood == 0} {
    putidx $idx "- Channel AVALANCHE flood:  Off"
  } else {
    putidx $idx "- Channel AVALANCHE flood:  [lindex $sl_avflood 0] in [lindex $sl_avflood 1] secs"
  }
  if {$sl_boflood == 0} {
    putidx $idx "- Channel BOGUS flood:      Off"
  } else {
    putidx $idx "- Channel BOGUS flood:      [lindex $sl_boflood 0] in [lindex $sl_boflood 1] secs"
  }
  if {$sl_jflood == 0} {
    putidx $idx "- Channel JOIN-PART flood:  Off"
  } else {
    putidx $idx "- Channel JOIN-PART flood:  [lindex $sl_jflood 0] in [lindex $sl_jflood 1] secs"
  }
  if {$sl_nkflood == 0} {
    putidx $idx "- Channel NICK flood:       Off"
  } else {
    putidx $idx "- Channel NICK flood:       [lindex $sl_nkflood 0] in [lindex $sl_nkflood 1] secs"
  }
  if {$sl_ilocktime == 0} {
    putidx $idx "- Channel +i locktime:      Indefinite"
  } else {
    putidx $idx "- Channel +i locktime:      $sl_ilocktime secs"
  }
  if {$sl_mlocktime == 0} {
    putidx $idx "- Channel +m locktime:      Indefinite"
  } else {
    putidx $idx "- Channel +m locktime:      $sl_mlocktime secs"
  }
  if {$sl_ban > 0 && $sl_ban < 120} {
    putidx $idx "- Channel flood bans:       $sl_ban mins"
  } elseif {$sl_ban >= 120} {
    putidx $idx "- Channel flood bans:       [expr $sl_ban / 60] hrs"
  } else {
    putidx $idx "- Channel flood bans:       Disabled"
  }
  if {$sl_globalban && $sl_ban > 0} {
    putidx $idx "- Ban type:                 Global"
  } elseif {!$sl_globalban && $sl_ban > 0} {
    putidx $idx "- Ban type:                 Channel-specific"
  }
  if {$sl_igtime > 0 && $sl_igtime < 120} {
    putidx $idx "- Flooder ignores:          $sl_igtime mins"
  } elseif {$sl_igtime >= 120} {
    putidx $idx "- Flooder ignores:          [expr $sl_igtime / 60] hrs"
  } else {
    putidx $idx "- Flooder ignores:          Permanent"
  }
  if {$sl_ban > 0} {
    putidx $idx "- Kicks per line:           $sl_kick"
  }
  if {$sl_bfmaxbans == 0} {
    putidx $idx "- Maximum channel bans:     Disabled"
  } else {
    putidx $idx "- Maximum channel bans:     $sl_bfmaxbans"
  }
  if {$sl_note != ""} {
    putidx $idx "- Flood notification:       Notifying $sl_note"
  } else {
    putidx $idx "- Flood notification:       Off"
  }
  if {$sl_lockcmds == 0} {
    putidx $idx "- Public lc/uc commands:    Disabled"
  } elseif {$sl_lockcmds == 1} {
    putidx $idx "- Public lc/uc commands:    Enabled (+$sl_lockflag users, ops not required)"
  } elseif {$sl_lockcmds == 2} {
    putidx $idx "- Public lc/uc commands:    Enabled (+$sl_lockflag users, ops required)"
  }
  if {$sl_bxsimul} {
    putidx $idx "- BitchX simulation:        On"
  } elseif {!$sl_bxsimul} {
    putidx $idx "- BitchX simulation:        Off"
  }
}

if {$sl_bxsimul} {
  bind raw - 002 sl_bxserverjoin
  set sl_bxonestack 0
  set sl_bxversion "75p1+"
  set sl_bxsystem "*IX"
  catch { set sl_bxsystem  "[exec uname -s -r]" }
  set sl_bxwhoami "$username"
  catch { set sl_bxwhoami  "[exec whoami]" }
  set sl_bxmachine ""
  catch { set sl_bxmachine "[exec uname -n]" }
  proc sl_bxserverjoin {from keyword arg} {
    global botnick sl_bxjointime sl_bxisaway
    set sl_bxjointime [unixtime]
    set sl_bxisaway 0
  }
  proc sl_bxaway {} {
    global sl_bxisaway botnick
    if {!$sl_bxisaway} {
      putserv "AWAY :is away: (Auto-Away after 10 mins) \[BX-MsgLog On\]"
      set sl_bxisaway 1
      putlog "sentinel: $botnick set AWAY"
    } else {
      putserv "AWAY"
      set sl_bxisaway 0
      putlog "sentinel: $botnick set BACK"
    }
    timer [expr [rand 300] + 10] sl_bxaway
  }
}

if {$sl_bxsimul && ![info exists sl_bxawaytmr]} {
  set sl_bxawaytmr 1
  set sl_bxisaway 0
  timer [expr [rand 300] + 10] sl_bxaway
}

proc sl_setarray {nick uhost hand chan} {
  global botnick sl_avbanhost sl_avbannick sl_avqueue sl_bfull sl_bobanhost sl_bobannick sl_boqueue sl_ccbanhost sl_ccbannick sl_ccqueue sl_flooded sl_jbanhost sl_jbannick sl_jqueue sl_nkbanhost sl_nkqueue sl_pbanhost sl_pbannick sl_pqueue
  if {$nick != $botnick} {return 0}
  set chan [string tolower $chan]
  foreach utimer [utimers] {
    if {[string match "sl_*queuereset $chan" [lindex $utimer 1]]} {
      killutimer [lindex $utimer 2]
    } elseif {[string match "sl_*banqueue $chan" [lindex $utimer 1]]} {
      killutimer [lindex $utimer 2]
    }
  }
  set sl_flooded($chan) 0
  set sl_ccqueue($chan) 0
  set sl_ccbanhost($chan) ""
  set sl_ccbannick($chan) ""
  set sl_avqueue($chan) 0
  set sl_avbanhost($chan) ""
  set sl_avbannick($chan) ""
  set sl_nkqueue($chan) 0
  set sl_nkbanhost($chan) ""
  set sl_boqueue($chan) 0
  set sl_bobanhost($chan) ""
  set sl_bobannick($chan) ""
  set sl_jqueue($chan) 0
  set sl_jbanhost($chan) ""
  set sl_jbannick($chan) ""
  set sl_pqueue($chan) 0
  set sl_pbanhost($chan) ""
  set sl_pbannick($chan) ""
  set sl_bfull($chan) 0
}

set sl_bflooded 0
set sl_bcqueue 0
set sl_bmqueue 0

set sl_bcflood [split $sl_bcflood :]
set sl_bmflood [split $sl_bmflood :]
set sl_ccflood [split $sl_ccflood :]
set sl_avflood [split $sl_avflood :]
set sl_boflood [split $sl_boflood :]
set sl_jflood [split $sl_jflood :]
set sl_nkflood [split $sl_nkflood :]

if {$sl_ilocktime > 0 && $sl_ilocktime < 15} {
  set $sl_ilocktime 15
}
if {$sl_mlocktime > 0 && $sl_mlocktime < 15} {
  set $sl_mlocktime 15
}

set trigger-on-ignore 0

if {$numversion >= 1032100} {
  set kick-bogus 0
}

if {$numversion >= 1032400} {
  set ban-bogus 0
  set kick-fun 0
  set ban-fun 0
}

if {$numversion >= 1032500} {
  set ctcp-mode 0
}

set sl_ver "v1.54"

bind pub $sl_lockflag|$sl_lockflag lc sl_lc
bind pub $sl_lockflag|$sl_lockflag uc sl_uc
if {!$sl_lockcmds} {
  unbind pub $sl_lockflag|$sl_lockflag lc sl_lc
  unbind pub $sl_lockflag|$sl_lockflag uc sl_uc
}
bind dcc m|m sentinel sl_dcc
bind ctcp - CLIENTINFO sl_ctcp
bind ctcp - USERINFO sl_ctcp
bind ctcp - VERSION sl_ctcp
bind ctcp - FINGER sl_ctcp
bind ctcp - ERRMSG sl_ctcp
bind ctcp - ECHO sl_ctcp
bind ctcp - INVITE sl_ctcp
bind ctcp - WHOAMI sl_ctcp
bind ctcp - OP sl_ctcp
bind ctcp - OPS sl_ctcp
bind ctcp - UNBAN sl_ctcp
bind ctcp - PING sl_ctcp
bind ctcp - TIME sl_ctcp
bind msgm - * sl_bmflood
bind raw - NOTICE sl_avflood
bind raw - PRIVMSG sl_avflood
bind nick - * sl_nkflood
bind join - * sl_setarray
bind join - * sl_jflood
bind part - * sl_pflood
bind kick - * sl_pfloodk
bind flud - * sl_flud
bind mode - * sl_mode

putlog "Loaded sentinel.tcl $sl_ver by slennox"
