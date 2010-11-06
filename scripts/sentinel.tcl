# sentinel.tcl v2.70 (15 April 2002)
# Copyright 1998-2002 by slennox
# slennox's eggdrop page - http://www.egghelp.org/

# Flood protection system for eggdrop, with integrated BitchX CTCP
# simulation. This script is designed to provide strong protection for your
# bot and channels against large floodnets and proxy floods.
#
# Note that this script was developed on the eggdrop 1.3, 1.4, and 1.6
# series and may not work properly on other versions.
#
# v2.00 - New standalone release. Contains refinements and features from
#         the netbots.tcl version of sentinel.tcl.
# v2.50 - Locktimes of less than 30 were erroneously allowed.
#         putquick -next is now supported (eggdrop 1.5+) for faster channel
#         lock.
#       - Ban mechanism now checks if flooders are coming from the same
#         domain or ident and performs wildcard bans instead of banning
#         each IP/host individually.
#       - Added tsunami detection to the avalanche flood detection system.
#       - Variables are now cleared after removing a channel.
#       - Removed unnecessary botonchan checks throughout components where
#         botisop already checks for that.
#       - Removed all unnecessary use of parentheses.
# v2.60 - Modified putquick compatibility proc.
#       - Added sl_wideban option to make domain/ident bans optional.
#       - Fixed typos in various ban-related functions.
#       - Unused procs are now unloaded.
#       - Wildcard bans covering domains/idents were not doing proper
#         checks on join-part flooders.
# v2.70 - Fixed "allbans" error which could occur when sl_wideban is
#         disabled.
#       - Added sl_masktype option, currently offering three different
#         ban/ignore mask types.
#       - Merged unsets in sl_unsetarray.
#       - Changed use of "split" in timers to the less ugly "list".
#
# sentinel.tcl is centered around its channel lock mechanism. It sets the
# channel +mi (moderated and invite-only) whenever a substantial flood on
# the channel is detected. This ensures channel stability when flooded,
# allowing the bots to deal with the flood as smoothly as possible.
# sentinel.tcl detects the following types of floods:
#
# * Channel CTCP floods. This is the most common type of flood, and will
#   often make users quit from the channel with 'Excess Flood'. A quick
#   channel lock can prevent this.
# * Channel join-part floods. A common type of channel flood in which many
#   floodbots cycle the channel.
# * Channel nick floods. Nick floods are unique in that they can occur even
#   after a channel is locked. sentinel has special mechanisms to deal with
#   this as effectively as possible.
# * Channel avalanche/tsunami floods. While the avalanche flood is quite
#   uncommon these days, tsunami floods are often used to seriously lag
#   mIRC and other clients using control codes (colour, bold, underline,
#   etc.)
# * Channel text floods. Not small text floods - but when hundreds of
#   messages are sent to the channel within a short period. Detected to
#   stop really aggressive text floods and reduce the possibility of the
#   bot crashing or consuming excessive CPU.
#
# sentinel also has additional protection features for the bot and channel:
#
# * Bogus username detection. Users with annoying bogus characters in their
#   ident are banned. A channel lock is applied if multiple join in a short
#   period.
# * Full ban list detection. During a serious flood, the ban list may
#   become full. If this happens, the bots may start kick flooding since
#   they cannot ban. If the ban list is full, the channel will be set +i.
# * Bot CTCP flood protection. Protects the bot if it is CTCP flooded.
# * Bot MSG flood protection. Protects the bot if it's flooded with MSGs or
#   MSG commands.
# * Automatic bans and ignores. During a flood, sentinel will compile a
#   list of the flooders, then kick-ban them after the channel has been
#   locked.
# * BitchX simulation. This has been built-in mainly because you cannot use
#   a third-party BitchX simulator with sentinel (only one script at a time
#   can have control of CTCPs). sentinel provides accurate simulation of
#   BitchX 75p1+ and 75p3+ CTCP replies and AWAY mode.
# * Public commands (lc and uc) and DCC commands (.lock and .unlock) for
#   locking/unlocking channels.
# * DCC command .sentinel displays current settings.
#
# Important Notes:
# - Make sure no bots are enforcing channel mode -i and/or -m.
# - Bans are added to the bot's internal ban list, and expire after 24
#   hours by default. If you have +dynamicbans set, the bans may be removed
#   from the channel much sooner than this, but the ban will remain in the
#   bot's internal ban list until it expires.
# - For greater protection against large channel floods, I recommend you
#   also use a channel limiter script, such as chanlimit.tcl.
# - There is a trade-off between convenience and security. The more
#   automation you enable, the more stress the bot will be under during a
#   flood and the more stuff it will be sending to the server.
# - Where security is paramount, have one or two bots that aren't running
#   sentinel.tcl. Since sentinel.tcl is a complex script with many
#   automated and convenience features, there is a potential for
#   vulnerabilities.

# The following flood settings are in number:seconds format, 0:0 to
# disable.

# Bot CTCP flood.
set sl_bcflood 5:30

# Bot MSG flood.
set sl_bmflood 6:20

# Channel CTCP flood.
set sl_ccflood 5:20

# Channel avalanche/tsunami flood.
set sl_avflood 6:20

# Channel text flood.
set sl_txflood 80:30

# Channel bogus username join flood.
set sl_boflood 4:20

# Channel join-part flood.
set sl_jflood 6:20

# Channel nick flood.
set sl_nkflood 6:20

# Flood setting notes:
# - Don't fiddle too much with the seconds field in the flood settings, as
#   it can reduce the effectiveness of the script. The seconds field should
#   set in the 20-60 seconds range.
# - Avalanche/tsunmami flood detection may be CPU intensive on a busy
#   channel, although I don't think it's a big deal on most systems. If
#   you're concerned about CPU usage you may wish to disable it, perhaps
#   leaving it enabled on one bot. Disabling text flood protection can
#   further reduce CPU usage.
# - On bots with avalanche/tsunami flood detection enabled, it's
#   recommended that you also enable text flood detection to cap CPU usage
#   during a flood.
# - If you enable nick flood detection, it's strongly recommended that it
#   be enabled on all bots that have sentinel.tcl loaded. This is required
#   for effective nick flood handling.

# Specify the number of control characters that must be in a line before
# it's counted by the tsunami flood detector. For efficiency reasons,
# tsunami detection is implemented with avalanche detection, so sl_avflood
# must be enabled for tsunami detection to be active. Setting this to 0
# will disable tsunami detection.
set sl_tsunami 10
# Valid settings: 0 to disable, otherwise 1 or higher.

# Length of time in minutes to ban channel flooders. This makes the bot
# perform kicks and bans on flooders after the channel lock. Because of the
# reactive nature of automatic bans, you should disable this on at least
# one bot for the most effective protection.
set sl_ban 1440
# Valid settings: 0 to disable, otherwise 1 or higher.

# Length of time in minutes of on-join bans for bogus usernames. For the
# most effective protection, you should disable this on at least one bot.
set sl_boban 1440
# Valid settings: 0 to disable, otherwise 1 or higher.

# Set global bans on channel flooders and bogus usernames?
set sl_globalban 0
# Valid settings: 1 for global bans, 0 for channel-specific bans.

# When processing a list of flooders, sentinel.tcl compares the hosts to
# see if multiple flooders are coming from a particular domain/IP or using
# the same ident (e.g. different vhosts on a single user account). If
# multiple flooders come from the same domain/IP, or if multiple flooders
# have the same ident, then the whole domain/IP (i.e. *!*@*.domain.com or
# *!*@555.555.555.*) or ident (i.e. *!*username@*) is banned. If you
# disable this option, all bans will be in *!*@machine.domain.com and
# *!*@555.555.555.555 format.
set sl_wideban 1
# Valid settings: 1 to enable, 0 to disable.

# Maximum number of bans allowed in the bot's ban list before sentinel will
# stop adding new bans. This prevents the bot from adding hundreds of bans
# on really large floods. Note that this has nothing to do with the channel
# ban list.
set sl_banmax 100
# Valid settings: 1 or higher.

# Length of time in minutes to ignore bot flooders. On bots with sl_ban
# active, channel flooders are also added to the ignore list.
set sl_igtime 240
# Valid settings: 1 or higher.

# Select the type of hostmask to use when banning and/or ignoring flooders.
# There are three to choose from:
#   0 - *!*@machine.domain.com / *!*@555.555.555.555
#   1 - *!*ident@machine.domain.com / *!*ident@555.555.555.555
#   2 - *!*ident@*.domain.com / *!*ident@555.555.555.*
# The default option, 0, is strongly recommended for most situations, and
# provides the best level of protection. The other two options are provided
# mainly for special cases.
set sl_masktype 0
# Valid settings: 0, 1, or 2, depending on the hostmask type you wish to use.

# Length of time in seconds to set channel +i if flooded. If set to 0, +i
# will not be removed automatically.
set sl_ilocktime 120
# Valid settings: 0 to prevent +i being removed automatically, otherwise 30
# or higher.

# Length of time in seconds to set channel +m if flooded. If set to 0, +m
# will not be removed automatically.
set sl_mlocktime 60
# Valid settings: 0 to prevent +m being removed automatically, otherwise 30
# or higher.

# On small floods (two flooders or less), remove the +mi shortly after bans
# have been set, instead of waiting for the locktimes to expire? This
# prevents unnecessary extended locks on small floods. This setting is only
# used by bots with sl_ban enabled.
set sl_shortlock 0
# Valid settings: 0 to disable, 1 to enable.

# Number of bans to allow in the channel ban list before setting the
# channel +i. If enabled, this should preferably be set to just below the
# maximum number of bans allowed.
set sl_bfmaxbans 19
# Valid settings: 0 to disable +i on full ban list, otherwise 1 or higher.

# List of users to send a note to when channel is flooded, bot is flooded,
# or ban list becomes full.
set sl_note "YourNick"
# Valid settings: one user like "Tom", a list of users like
# "Tom Dick Harry", or "" to specify that no notes are sent.

# Notice to send to channel when locked due to flood.
set sl_cfnotice "Channel locked temporarily due to flood, sorry for any inconvenience this may cause :-)"
# Valid settings: a text string, or set it to "" to disable.

# Notice to send to channel when locked due to full ban list.
set sl_bfnotice "Channel locked temporarily due to full ban list, sorry for any inconvenience this may cause :-)"
# Valid settings: a text string, or set it to "" to disable.

# Enable 'lc' and 'uc' public commands for locking/unlocking channel?
set sl_lockcmds 2
# Valid settings: 0 to disable, 1 to enable, 2 to require user to be opped
# on the channel to use the command.

# Users with these flags are allowed to use lc/uc public commands, and
# .lock/.unlock DCC commands.
set sl_lockflags "o"
# Valid settings: one flag like "n", or a list of flags like "fo" (means
# 'f OR o').

# Enable BitchX CTCP and AWAY simulation?
set sl_bxsimul 0
# Valid settings: 1 to enable, 0 to disable.


# Don't edit below unless you know what you're doing.

if {$numversion < 1032400} {
  proc botonchan {chan} {
    global botnick
    if {![validchan $chan]} {
      error "illegal channel: $chan"
    } elseif {![onchan $botnick $chan]} {
      return 0
    }
    return 1
  }
  proc putquick {text args} {
    putserv $text
  }
}

proc sl_ctcp {nick uhost hand dest key arg} {
  global botnet-nick botnick realname sl_ban sl_bflooded sl_bcflood sl_bcqueue sl_bxjointime sl_bxmachine sl_bxonestack sl_bxsimul sl_bxsystem sl_bxversion sl_bxwhoami sl_ccbanhost sl_ccbannick sl_ccflood sl_ccqueue sl_flooded sl_locked sl_note
  set chan [string tolower $dest]
  if {[lsearch -exact $sl_ccflood 0] == -1 && [validchan $chan] && ![isop $nick $chan]} {
    if {$nick == $botnick} {return 0}
    if {$sl_ban && !$sl_locked($chan) && ![matchattr $hand f|f $chan]} {
      lappend sl_ccbannick($chan) $nick ; lappend sl_ccbanhost($chan) [string tolower $uhost]
      utimer [lindex $sl_ccflood 1] [list sl_ccbanqueue $chan]
    }
    if {$sl_flooded($chan)} {return 1}
    incr sl_ccqueue($chan)
    utimer [lindex $sl_ccflood 1] [list sl_ccqueuereset $chan]
    if {$sl_ccqueue($chan) >= [lindex $sl_ccflood 0]} {
      sl_lock $chan "CTCP flood" ${botnet-nick} ; return 1
    }
    if {$sl_bflooded} {return 1}
  } elseif {[lindex $sl_bcflood 0] && $dest == $botnick} {
    if {$sl_bflooded} {
      sl_ignore [string tolower $uhost] $hand "CTCP flooder" ; return 1
    }
    incr sl_bcqueue
    utimer [lindex $sl_bcflood 1] {incr sl_bcqueue -1}
    if {$sl_bcqueue >= [lindex $sl_bcflood 0]} {
      putlog "sentinel: CTCP flood detected on me! Stopped answering CTCPs temporarily."
      set sl_bflooded 1
      utimer [lindex $sl_bcflood 1] {set sl_bflooded 0}
      if {[info commands sendnote] != ""} {
        foreach recipient $sl_note {
          if {[validuser $recipient]} {
            sendnote SENTINEL $recipient "Bot was CTCP flooded."
          }
        }
      }
      return 1
    }
  }

  if {!$sl_bxsimul} {return 0}
  if {$sl_bxonestack} {return 1}
  set sl_bxonestack 1 ; utimer 2 {set sl_bxonestack 0}
  switch -exact -- $key {
    "CLIENTINFO" {
      set bxcmd [string toupper $arg]
      switch -exact -- $bxcmd {
        "" {putserv "NOTICE $nick :\001CLIENTINFO SED UTC ACTION DCC CDCC BDCC XDCC VERSION CLIENTINFO USERINFO ERRMSG FINGER TIME PING ECHO INVITE WHOAMI OP OPS UNBAN IDENT XLINK UPTIME  :Use CLIENTINFO <COMMAND> to get more specific information\001"}
        "SED" {putserv "NOTICE $nick :\001CLIENTINFO SED contains simple_encrypted_data\001"}
        "UTC" {putserv "NOTICE $nick :\001CLIENTINFO UTC substitutes the local timezone\001"}
        "ACTION" {putserv "NOTICE $nick :\001CLIENTINFO ACTION contains action descriptions for atmosphere\001"}
        "DCC" {putserv "NOTICE $nick :\001CLIENTINFO DCC requests a direct_client_connection\001"}
        "CDCC" {putserv "NOTICE $nick :\001CLIENTINFO CDCC checks cdcc info for you\001"}
        "BDCC" {putserv "NOTICE $nick :\001CLIENTINFO BDCC checks cdcc info for you\001"}
        "XDCC" {putserv "NOTICE $nick :\001CLIENTINFO XDCC checks cdcc info for you\001"}
        "VERSION" {putserv "NOTICE $nick :\001CLIENTINFO VERSION shows client type, version and environment\001"}
        "CLIENTINFO" {putserv "NOTICE $nick :\001CLIENTINFO CLIENTINFO gives information about available CTCP commands\001"}
        "USERINFO" {putserv "NOTICE $nick :\001CLIENTINFO USERINFO returns user settable information\001"}
        "ERRMSG" {putserv "NOTICE $nick :\001CLIENTINFO ERRMSG returns error messages\001"}
        "FINGER" {putserv "NOTICE $nick :\001CLIENTINFO FINGER shows real name, login name and idle time of user\001"}
        "TIME" {putserv "NOTICE $nick :\001CLIENTINFO TIME tells you the time on the user's host\001"}
        "PING" {putserv "NOTICE $nick :\001CLIENTINFO PING returns the arguments it receives\001"}
        "ECHO" {putserv "NOTICE $nick :\001CLIENTINFO ECHO returns the arguments it receives\001"}
        "INVITE" {putserv "NOTICE $nick :\001CLIENTINFO INVITE invite to channel specified\001"}
        "WHOAMI" {putserv "NOTICE $nick :\001CLIENTINFO WHOAMI user list information\001"}
        "OP" {putserv "NOTICE $nick :\001CLIENTINFO OP ops the person if on userlist\001"}
        "OPS" {putserv "NOTICE $nick :\001CLIENTINFO OPS ops the person if on userlist\001"}
        "UNBAN" {putserv "NOTICE $nick :\001CLIENTINFO UNBAN unbans the person from channel\001"}
        "IDENT" {putserv "NOTICE $nick :\001CLIENTINFO IDENT change userhost of userlist\001"}
        "XLINK" {putserv "NOTICE $nick :\001CLIENTINFO XLINK x-filez rule\001"}
        "UPTIME" {putserv "NOTICE $nick :\001CLIENTINFO UPTIME my uptime\001"}
        "default" {putserv "NOTICE $nick :\001ERRMSG CLIENTINFO: $arg is not a valid function\001"}
      }
      return 1
    }
    "VERSION" {
      putserv "NOTICE $nick :\001VERSION \002BitchX-$sl_bxversion\002 by panasync \002-\002 $sl_bxsystem :\002 Keep it to yourself!\002\001"
      return 1
    }
    "USERINFO" {
      putserv "NOTICE $nick :\001USERINFO \001"
      return 1
    }
    "FINGER" {
      putserv "NOTICE $nick :\001FINGER $realname ($sl_bxwhoami@$sl_bxmachine) Idle [expr [unixtime] - $sl_bxjointime] seconds\001"
      return 1
    }
    "PING" {
      putserv "NOTICE $nick :\001PING $arg\001"
      return 1
    }
    "ECHO" {
      if {[validchan $chan]} {return 1}
      putserv "NOTICE $nick :\001ECHO [string range $arg 0 59]\001"
      return 1
    }
    "ERRMSG" {
      if {[validchan $chan]} {return 1}
      putserv "NOTICE $nick :\001ERRMSG [string range $arg 0 59]\001"
      return 1
    }
    "INVITE" {
      if {$arg == "" || [validchan $chan]} {return 1}
      set chanarg [lindex [split $arg] 0]
      if {((($sl_bxversion == "75p1+") && ([string trim [string index $chanarg 0] "#+&"] == "")) || (($sl_bxversion == "75p3+") && ([string trim [string index $chanarg 0] "#+&!"] == "")))} {
        if {[validchan $chanarg]} {
          putserv "NOTICE $nick :\002BitchX\002: Access Denied"
        } else {
          putserv "NOTICE $nick :\002BitchX\002: I'm not on that channel"
        }
      }
      return 1
    }
    "WHOAMI" {
      if {[validchan $chan]} {return 1}
      putserv "NOTICE $nick :\002BitchX\002: Access Denied"
      return 1
    }
    "OP" -
    "OPS" {
      if {$arg == "" || [validchan $chan]} {return 1}
      putserv "NOTICE $nick :\002BitchX\002: I'm not on [lindex [split $arg] 0], or I'm not opped"
      return 1
    }
    "UNBAN" {
      if {$arg == "" || [validchan $chan]} {return 1}
      if {[validchan [lindex [split $arg] 0]]} {
        putserv "NOTICE $nick :\002BitchX\002: Access Denied"
      } else {
        putserv "NOTICE $nick :\002BitchX\002: I'm not on that channel"
      }
      return 1
    }
  }
  return 0
}

proc sl_bmflood {nick uhost hand text} {
  global sl_bmflood sl_bflooded sl_bmqueue sl_note
  if {[matchattr $hand b] && [string tolower [lindex [split $text] 0]] == "go"} {return 0}
  if {$sl_bflooded} {
    sl_ignore [string tolower $uhost] $hand "MSG flooder" ; return 0
  }
  incr sl_bmqueue
  utimer [lindex $sl_bmflood 1] {incr sl_bmqueue -1}
  if {$sl_bmqueue >= [lindex $sl_bmflood 0]} {
    putlog "sentinel: MSG flood detected on me! Stopped answering MSGs temporarily."
    set sl_bflooded 1
    utimer [lindex $sl_bmflood 1] {set sl_bflooded 0}
    if {[info commands sendnote] != ""} {
      foreach recipient $sl_note {
        if {[validuser $recipient]} {
          sendnote SENTINEL $recipient "Bot was MSG flooded."
        }
      }
    }
  }
  return 0
}

proc sl_avflood {from keyword arg} {
  global botnet-nick botnick sl_ban sl_avbanhost sl_avbannick sl_avflood sl_avqueue sl_flooded sl_locked sl_txflood sl_txqueue
  set arg [split $arg]
  set chan [string tolower [lindex $arg 0]]
  if {![validchan $chan]} {return 0}
  set nick [lindex [split $from !] 0]
  if {$nick == $botnick || $nick == "" || [string match *.* $nick]} {return 0}
  if {![onchan $nick $chan] || [isop $nick $chan]} {return 0}
  if {!$sl_flooded($chan) && [lsearch -exact $sl_txflood 0] == -1} {
    incr sl_txqueue($chan)
    if {$sl_txqueue($chan) >= [lindex $sl_txflood 0]} {
      sl_lock $chan "TEXT flood" ${botnet-nick}
    }
  }
  set text [join [lrange $arg 1 end]]
  if {[sl_checkaval $text] && [lsearch -exact $sl_avflood 0] == -1} {
    set uhost [string trimleft [getchanhost $nick $chan] "~+-^="]
    set hand [nick2hand $nick $chan]
    if {$sl_ban && !$sl_locked($chan) && $nick != $botnick && ![matchattr $hand f|f $chan]} {
      lappend sl_avbannick($chan) $nick ; lappend sl_avbanhost($chan) [string tolower $uhost]
      utimer [lindex $sl_avflood 1] [list sl_avbanqueue $chan]
    }
    if {$sl_flooded($chan)} {return 0}
    incr sl_avqueue($chan)
    utimer [lindex $sl_avflood 1] [list sl_avqueuereset $chan]
    if {$sl_avqueue($chan) >= [lindex $sl_avflood 0]} {
      sl_lock $chan "AVALANCHE/TSUNAMI flood" ${botnet-nick}
    }
  }
  return 0
}

proc sl_checkaval {text} {
  global sl_tsunami
  if {[regsub -all -- "\001|\007" $text "" temp] >= 3} {return 1}
  if {$sl_tsunami && [regsub -all -- "\002|\003|\017|\026|\037" $text "" temp] >= $sl_tsunami} {return 1}
  return 0
}

proc sl_nkflood {nick uhost hand chan newnick} {
  global botnet-nick botnick sl_ban sl_banmax sl_flooded sl_globalban sl_locked sl_nickkick sl_nkbanhost sl_nkflood sl_nkflooding sl_nkqueue
  set chan [string tolower $chan]
  if {[isop $newnick $chan]} {return 0}
  if {$sl_ban && !$sl_locked($chan) && $nick != $botnick && ![matchattr $hand f|f $chan]} {
    lappend sl_nkbanhost($chan) [string tolower $uhost]
    utimer [lindex $sl_nkflood 1] [list sl_nkbanqueue $chan]
  }
  if {!$sl_nickkick && $sl_flooded($chan) && $sl_locked($chan)} {
    putserv "KICK $chan $newnick :NICK flooder"
    set sl_nickkick 1 ; set sl_nkflooding($chan) [unixtime]
    if {$sl_ban} {
      set bhost [string tolower [sl_masktype $uhost]]
      if {$sl_globalban} {
        if {[llength [banlist]] < $sl_banmax && ![isban $bhost] && ![matchban $bhost]} {
          newban $bhost sentinel "NICK flooder" $sl_ban
        }
      } else {
        if {[llength [banlist $chan]] < $sl_banmax && ![isban $bhost $chan] && ![matchban $bhost $chan]} {
          newchanban $chan $bhost sentinel "NICK flooder" $sl_ban
        }
      }
    }
    utimer [expr [rand 2] + 3] {set sl_nickkick 0}
    return 0
  }
  if {$sl_flooded($chan)} {return 0}
  incr sl_nkqueue($chan)
  utimer [lindex $sl_nkflood 1] [list sl_nkqueuereset $chan]
  if {$sl_nkqueue($chan) >= [lindex $sl_nkflood 0]} {
    sl_lock $chan "NICK flood" ${botnet-nick}
  }
  return 0
}

proc sl_jflood {nick uhost hand chan} {
  global botnet-nick botnick sl_ban sl_banmax sl_boban sl_bobanhost sl_bobannick sl_boflood sl_boqueue sl_flooded sl_globalban sl_jbanhost sl_jbannick sl_jflood sl_jqueue sl_locked sl_pqueue
  if {$nick == $botnick} {
    sl_setarray $chan
  } else {
    set ihost [string tolower [sl_masktype $uhost]]
    if {[isignore $ihost]} {
      killignore $ihost
    }
    set chan [string tolower $chan]
    if {[lsearch -exact $sl_boflood 0] == -1 && [sl_checkbogus [lindex [split $uhost @] 0]]} {
      if {!$sl_locked($chan) && ![matchattr $hand f|f $chan]} {
        set bhost [string tolower [sl_masktype $uhost]]
        if {$sl_boban && [botisop $chan] && !$sl_flooded($chan)} {
          putserv "KICK $chan $nick :BOGUS username"
          if {$sl_globalban} {
            if {[llength [banlist]] < $sl_banmax && ![isban $bhost] && ![matchban $bhost]} {
              newban $bhost sentinel "BOGUS username" $sl_boban
            }
          } else {
            if {[llength [banlist $chan]] < $sl_banmax && ![isban $bhost $chan] && ![matchban $bhost $chan]} {
              newchanban $chan $bhost sentinel "BOGUS username" $sl_boban
            }
          }
        }
        if {$sl_ban} {
          lappend sl_bobannick($chan) $nick ; lappend sl_bobanhost($chan) [string tolower $uhost]
          utimer [lindex $sl_boflood 1] [list sl_bobanqueue $chan]
        }
      }
      if {!$sl_flooded($chan)} {
        incr sl_boqueue($chan)
        utimer [lindex $sl_boflood 1] [list sl_boqueuereset $chan]
        if {$sl_boqueue($chan) >= [lindex $sl_boflood 0]} {
          sl_lock $chan "BOGUS joins" ${botnet-nick}
        }
      }
    }
    if {[lsearch -exact $sl_jflood 0] == -1} {
      if {$sl_ban && !$sl_locked($chan) && ![matchattr $hand f|f $chan]} {
        lappend sl_jbannick($chan) $nick ; lappend sl_jbanhost($chan) [string tolower $uhost]
        utimer [lindex $sl_jflood 1] [list sl_jbanqueue $chan]
      }
      if {$sl_flooded($chan)} {return 0}
      incr sl_jqueue($chan)
      utimer [lindex $sl_jflood 1] [list sl_jqueuereset $chan]
      if {$sl_jqueue($chan) >= [lindex $sl_jflood 0] && $sl_pqueue($chan) >= [lindex $sl_jflood 0]} {
        sl_lock $chan "JOIN-PART flood" ${botnet-nick}
      }
    }
  }
  return 0
}

proc sl_checkbogus {ident} {
  if {[regsub -all -- "\[^\041-\176\]" $ident "" temp] >= 1} {return 1}
  return 0
}

proc sl_pflood {nick uhost hand chan {msg ""}} {
  global botnick sl_ban sl_flooded sl_jflood sl_locked sl_pbanhost sl_pbannick sl_pqueue
  if {[lsearch -exact $sl_jflood 0] != -1} {return 0}
  if {$nick == $botnick} {
    if {![validchan $chan]} {
      timer 5 [list sl_unsetarray $chan]
    }
    return 0
  }
  set chan [string tolower $chan]
  if {$sl_ban && !$sl_locked($chan) && ![matchattr $hand f|f $chan]} {
    lappend sl_pbannick($chan) $nick ; lappend sl_pbanhost($chan) [string tolower $uhost]
    utimer [lindex $sl_jflood 1] [list sl_pbanqueue $chan]
  }
  if {$sl_flooded($chan)} {return 0}
  incr sl_pqueue($chan)
  utimer [lindex $sl_jflood 1] [list sl_pqueuereset $chan]
  return 0
}

proc sl_pfloodk {nick uhost hand chan kicked reason} {
  global botnick sl_flooded sl_jflood sl_pqueue
  if {[lsearch -exact $sl_jflood 0] != -1} {return 0}
  if {$kicked == $botnick} {return 0}
  set chan [string tolower $chan]
  if {$sl_flooded($chan)} {return 0}
  incr sl_pqueue($chan)
  utimer [lindex $sl_jflood 1] [list sl_pqueuereset $chan]
  return 0
}

proc sl_lock {chan flood detected} {
  global botnet-nick sl_bflooded sl_cfnotice sl_flooded sl_ilocktime sl_mlocktime sl_note
  if {[string tolower $detected] == [string tolower ${botnet-nick}]} {
    set sl_flooded($chan) 1 ; set sl_bflooded 1
    if {[botisop $chan]} {
      sl_quicklock $chan
      sl_killutimer "sl_unlock $chan *"
      sl_killutimer "set sl_bflooded 0"
      if {$sl_mlocktime} {
        utimer $sl_mlocktime [list sl_unlock $chan m]
      }
      if {$sl_ilocktime} {
        utimer $sl_ilocktime [list sl_unlock $chan i]
      }
      utimer 120 {set sl_bflooded 0}
      putlog "sentinel: $flood detected on $chan! Channel locked temporarily."
      if {$sl_cfnotice != ""} {
        puthelp "NOTICE $chan :$sl_cfnotice"
      }
    } else {
      putlog "sentinel: $flood detected on $chan! Cannot lock channel because I'm not opped."
      utimer 120 {set sl_bflooded 0}
    }
  } else {
    putlog "sentinel: $flood detected by $detected on $chan!"
  }
  if {[info commands sendnote] != ""} {
    foreach recipient $sl_note {
      if {[validuser $recipient]} {
        if {[string tolower $detected] == [string tolower ${botnet-nick}]} {
          sendnote SENTINEL $recipient "$flood detected on $chan."
        } else {
          sendnote SENTINEL $recipient "$flood detected by $detected on $chan."
        }
      }
    }
  }
  return 0
}

proc sl_unlock {chan umode} {
  global sl_bflooded sl_bfmaxbans sl_flooded sl_ilocktime sl_mlocktime sl_nkflooding
  if {[expr [unixtime] - $sl_nkflooding($chan)] < 12} {
    putlog "sentinel: nick flooding still in progress on $chan - not removing +mi yet.."
    set sl_flooded($chan) 1 ; set sl_bflooded 1
    sl_killutimer "sl_unlock $chan *"
    sl_killutimer "set sl_bflooded 0"
    utimer $sl_mlocktime [list sl_unlock $chan m] ; utimer $sl_ilocktime [list sl_unlock $chan i]
    utimer 120 {set sl_bflooded 0}
  } else {
    set sl_flooded($chan) 0
    if {![botisop $chan]} {return 0}
    if {$umode == "mi"} {
      putlog "sentinel: flood was small, performing early unlock.."
    }
    if {[string match *i* $umode] && [string match *i* [lindex [split [getchanmode $chan]] 0]]} {
      if {$sl_bfmaxbans && [llength [chanbans $chan]] >= $sl_bfmaxbans} {
        putlog "sentinel: not removing +i on $chan due to full ban list."
      } else {
        pushmode $chan -i
        putlog "sentinel: removed +i on $chan"
      }
    }
    if {[string match *m* $umode] && [string match *m* [lindex [split [getchanmode $chan]] 0]]} {
      pushmode $chan -m
      putlog "sentinel: removed +m on $chan"
    }
  }
  return 0
}

proc sl_mode {nick uhost hand chan mode victim} {
  global botnick sl_ban sl_bfmaxbans sl_bfnotice sl_bfull sl_flooded sl_locked sl_note sl_unlocked
  set chan [string tolower $chan]
  if {$mode == "+b" && $sl_bfmaxbans && !$sl_bfull($chan) && ![string match *i* [lindex [split [getchanmode $chan]] 0]] && [botisop $chan] && [llength [chanbans $chan]] >= $sl_bfmaxbans} {
    putserv "MODE $chan +i"
    set sl_bfull($chan) 1
    utimer 5 [list set sl_bfull($chan) 0]
    putlog "sentinel: locked $chan due to full ban list!"
    if {$sl_bfnotice != ""} {
      puthelp "NOTICE $chan :$sl_bfnotice"
    }
    if {[info commands sendnote] != ""} {
      foreach recipient $sl_note {
        if {[validuser $recipient]} {
          sendnote SENTINEL $recipient "Locked $chan due to full ban list."
        }
      }
    }
  } elseif {$mode == "+i" && $sl_flooded($chan)} {
    set sl_locked($chan) 1
    if {$sl_ban} {
      sl_killutimer "sl_*banqueue $chan"
      utimer 7 [list sl_dokicks $chan] ; utimer 16 [list sl_setbans $chan]
    }
  } elseif {$mode == "-i" || $mode == "-m"} {
    set sl_locked($chan) 0
    set sl_unlocked($chan) [unixtime]
    if {$sl_flooded($chan)} {
      set sl_flooded($chan) 0
      if {$mode == "-i"} {
        sl_killutimer "sl_unlock $chan i"
      } else {
        sl_killutimer "sl_unlock $chan m"
      }
      sl_killutimer "sl_unlock $chan mi"
      if {$nick != $botnick} {
        putlog "sentinel: $chan unlocked by $nick"
      }
    }
  }
  return 0
}

proc sl_dokicks {chan} {
  global sl_avbannick sl_bobannick sl_ccbannick sl_kflooders sl_jbannick sl_pbannick
  if {![botisop $chan]} {return 0}
  set sl_kflooders 0
  sl_kick $chan $sl_ccbannick($chan) "CTCP flooder" ; set sl_ccbannick($chan) ""
  sl_kick $chan $sl_avbannick($chan) "AVALANCHE/TSUNAMI flooder" ; set sl_avbannick($chan) ""
  sl_kick $chan $sl_bobannick($chan) "BOGUS username" ; set sl_bobannick($chan) ""
  set jklist $sl_jbannick($chan) ; set pklist $sl_pbannick($chan)
  if {$jklist != "" && $pklist != ""} {
    set klist ""
    foreach nick $jklist {
      if {[lsearch -exact $pklist $nick] != -1} {
        lappend klist $nick
      }
    }
    sl_kick $chan $klist "JOIN-PART flooder"
  }
  set sl_jbannick($chan) "" ; set sl_pbannick($chan) ""
  return 0
}

proc sl_kick {chan klist reason} {
  global sl_kflooders sl_kicks
  if {$klist != ""} {
    set kicklist ""
    foreach nick $klist {
      if {[lsearch -exact $kicklist $nick] == -1} {
        lappend kicklist $nick
      }
    }
    unset nick
    incr sl_kflooders [llength $kicklist]
    foreach nick $kicklist {
      if {[onchan $nick $chan] && ![onchansplit $nick $chan]} {
        lappend ksend $nick
        if {[llength $ksend] >= $sl_kicks} {
          putserv "KICK $chan [join $ksend ,] :$reason"
          unset ksend
        }
      }
    }
    if {[info exists ksend]} {
      putserv "KICK $chan [join $ksend ,] :$reason"
    }
  }
  return 0
}

proc sl_setbans {chan} {
  global sl_avbanhost sl_bobanhost sl_ccbanhost sl_kflooders sl_jbanhost sl_nkbanhost sl_pbanhost sl_shortlock sl_unlocked sl_wideban
  if {![botonchan $chan]} {return 0}
  set sl_ccbanhost($chan) [sl_dfilter $sl_ccbanhost($chan)]
  set sl_avbanhost($chan) [sl_dfilter $sl_avbanhost($chan)]
  set sl_nkbanhost($chan) [sl_dfilter $sl_nkbanhost($chan)]
  set sl_bobanhost($chan) [sl_dfilter $sl_bobanhost($chan)]
  set sl_jbanhost($chan) [sl_dfilter $sl_jbanhost($chan)]
  set sl_pbanhost($chan) [sl_dfilter $sl_pbanhost($chan)]
  set blist ""
  if {$sl_jbanhost($chan) != "" && $sl_pbanhost($chan) != ""} {
    foreach bhost $sl_jbanhost($chan) {
      if {[lsearch -exact $sl_pbanhost($chan) $bhost] != -1} {
        lappend blist $bhost
      }
    }
  }
  set allbans [sl_dfilter [concat $sl_ccbanhost($chan) $sl_avbanhost($chan) $sl_nkbanhost($chan) $sl_bobanhost($chan) $blist]]
  if {$sl_wideban} {
    sl_ban $chan [sl_dcheck $allbans] "MULTIPLE IDENT/HOST flooders"
  }
  sl_ban $chan $sl_ccbanhost($chan) "CTCP flooder" ; set sl_ccbanhost($chan) ""
  sl_ban $chan $sl_avbanhost($chan) "AVALANCHE/TSUNAMI flooder" ; set sl_avbanhost($chan) ""
  sl_ban $chan $sl_nkbanhost($chan) "NICK flooder" ; set sl_nkbanhost($chan) ""
  sl_ban $chan $sl_bobanhost($chan) "BOGUS username" ; set sl_bobanhost($chan) ""
  sl_ban $chan $blist "JOIN-PART flooder"
  set sl_jbanhost($chan) "" ; set sl_pbanhost($chan) ""
  if {$sl_shortlock && $sl_kflooders <= 2 && [llength $allbans] <= 2 && [expr [unixtime] - $sl_unlocked($chan)] > 120} {
    sl_killutimer "sl_unlock $chan *"
    utimer 10 [list sl_unlock $chan mi]
  }
  return 0
}

proc sl_dfilter {list} {
  set newlist ""
  foreach item $list {
    if {[lsearch -exact $newlist $item] == -1} {
      lappend newlist $item
    }
  }
  return $newlist
}

proc sl_dcheck {bhosts} {
  set blist ""
  foreach bhost $bhosts {
    set baddr [lindex [split [maskhost $bhost] "@"] 1]
    set bident [string trimleft [lindex [split $bhost "@"] 0] "~"]
    if {![info exists baddrs($baddr)]} {
      set baddrs($baddr) 1
    } else {
      incr baddrs($baddr)
    }
    if {![info exists bidents($bident)]} {
      set bidents($bident) 1
    } else {
      incr bidents($bident)
    }
  }
  foreach baddr [array names baddrs] {
    if {$baddrs($baddr) >= 2} {
      lappend blist *!*@$baddr
    }
  }
  foreach bident [array names bidents] {
    if {$bidents($bident) >= 2} {
      lappend blist *!*$bident@*
    }
  }
  return $blist
}

proc sl_ban {chan blist reason} {
  global sl_ban sl_banmax sl_globalban
  if {$blist != ""} {
    if {$sl_globalban} {
      foreach bhost $blist {
        if {![string match *!* $bhost]} {
          if {[matchban *!$bhost]} {continue}
          set bhost [sl_masktype $bhost]
          if {[isban $bhost]} {continue}
        } else {
          if {[isban $bhost]} {continue}
          foreach ban [banlist] {
            if {[lindex $ban 5] == "sentinel" && [string match $bhost [string tolower [lindex $ban 0]]]} {
              killban $ban
            }
          }
        }
        if {[llength [banlist]] >= $sl_banmax} {continue}
        newban $bhost sentinel $reason $sl_ban
        putlog "sentinel: banned $bhost ($reason)"
        sl_ignore $bhost * $reason
      }
    } else {
      foreach bhost $blist {
        if {![string match *!* $bhost]} {
          if {[matchban *!$bhost $chan]} {continue}
          set bhost [sl_masktype $bhost]
          if {[isban $bhost $chan]} {continue}
        } else {
          if {[isban $bhost $chan]} {continue}
          foreach ban [banlist $chan] {
            if {[lindex $ban 5] == "sentinel" && [string match $bhost [string tolower [lindex $ban 0]]]} {
              killchanban $chan $ban
            }
          }
        }
        if {[llength [banlist $chan]] >= $sl_banmax} {continue}
        newchanban $chan $bhost sentinel $reason $sl_ban
        putlog "sentinel: banned $bhost on $chan ($reason)"
        sl_ignore $bhost * $reason
      }
    }
  }
  return 0
}

proc sl_ignore {ihost hand flood} {
  global sl_igtime
  if {$hand != "*"} {
    foreach chan [channels] {
      if {[matchattr $hand f|f $chan]} {return 0}
    }
  }
  if {![string match *!* $ihost]} {
    foreach ignore [ignorelist] {
      if {[string match [string tolower [lindex $ignore 0]] $ihost]} {
        return 0
      }
    }
    set ihost [sl_masktype $ihost]
    if {[isignore $ihost]} {return 0}
  } else {
    if {[isignore $ihost]} {return 0}
    foreach ignore [ignorelist] {
      if {[lindex $ignore 4] == "sentinel" && [string match $ihost [string tolower [lindex $ignore 0]]]} {
        killignore $ignore
      }
    }
  }
  newignore $ihost sentinel $flood $sl_igtime
  putlog "sentinel: added $ihost to ignore list ($flood)"
  return 1
}

# queuereset procs allow all queue timers to be killed easily
proc sl_ccqueuereset {chan} {
  global sl_ccqueue
  incr sl_ccqueue($chan) -1
  return 0
}

proc sl_bcqueuereset {} {
  global sl_bcqueue
  incr sl_bcqueue -1
  return 0
}

proc sl_bmqueuereset {} {
  global sl_bmqueue
  incr sl_bmqueue -1
  return 0
}

proc sl_avqueuereset {chan} {
  global sl_avqueue
  incr sl_avqueue($chan) -1
  return 0
}

proc sl_txqueuereset {} {
  global sl_txqueue sl_txflood
  foreach chan [string tolower [channels]] {
    if {[info exists sl_txqueue($chan)]} {
      set sl_txqueue($chan) 0
    }
  }
  utimer [lindex $sl_txflood 1] sl_txqueuereset
  return 0
}

proc sl_nkqueuereset {chan} {
  global sl_nkqueue
  incr sl_nkqueue($chan) -1
  return 0
}

proc sl_boqueuereset {chan} {
  global sl_boqueue
  incr sl_boqueue($chan) -1
  return 0
}

proc sl_jqueuereset {chan} {
  global sl_jqueue
  incr sl_jqueue($chan) -1
  return 0
}

proc sl_pqueuereset {chan} {
  global sl_pqueue
  incr sl_pqueue($chan) -1
  return 0
}

proc sl_ccbanqueue {chan} {
  global sl_ccbanhost sl_ccbannick
  set sl_ccbannick($chan) [lrange sl_ccbannick($chan) 1 end] ; set sl_ccbanhost($chan) [lrange sl_ccbanhost($chan) 1 end]
  return 0
}

proc sl_avbanqueue {chan} {
  global sl_avbanhost sl_avbannick
  set sl_avbannick($chan) [lrange sl_avbannick($chan) 1 end] ; set sl_avbanhost($chan) [lrange sl_avbanhost($chan) 1 end]
  return 0
}

proc sl_nkbanqueue {chan} {
  global sl_nkbanhost
  set sl_nkbanhost($chan) [lrange sl_nkbanhost($chan) 1 end]
  return 0
}

proc sl_bobanqueue {chan} {
  global sl_bobanhost sl_bobannick
  set sl_bobannick($chan) [lrange sl_bobannick($chan) 1 end] ; set sl_bobanhost($chan) [lrange sl_bobanhost($chan) 1 end]
  return 0
}

proc sl_jbanqueue {chan} {
  global sl_jbanhost sl_jbannick
  set sl_jbannick($chan) [lrange sl_jbannick($chan) 1 end] ; set sl_jbanhost($chan) [lrange sl_jbanhost($chan) 1 end]
  return 0
}

proc sl_pbanqueue {chan} {
  global sl_pbanhost sl_pbannick
  set sl_pbannick($chan) [lrange sl_pbannick($chan) 1 end] ; set sl_pbanhost($chan) [lrange sl_pbanhost($chan) 1 end]
  return 0
}

proc sl_flud {nick uhost hand type chan} {
  global sl_flooded
  set chan [string tolower $chan]
  if {[validchan $chan] && $sl_flooded($chan)} {return 1}
  return 0
}

proc sl_lc {nick uhost hand chan arg} {
  global sl_lockcmds
  set chan [string tolower $chan]
  if {![botisop $chan]} {return 0}
  if {$sl_lockcmds == 2 && ![isop $nick $chan]} {return 0}
  sl_quicklock $chan
  putlog "sentinel: channel lock requested by $hand on $chan"
  return 0
}

proc sl_uc {nick uhost hand chan arg} {
  global sl_lockcmds
  set chan [string tolower $chan]
  if {![botisop $chan]} {return 0}
  if {$sl_lockcmds == 2 && ![isop $nick $chan]} {return 0}
  putserv "MODE $chan -mi"
  putlog "sentinel: channel unlock requested by $hand on $chan"
  return 0
}

proc sl_dcclc {hand idx arg} {
  global sl_lockflags
  putcmdlog "#$hand# lock $arg"
  set chan [lindex [split $arg] 0]
  if {$chan == "-all"} {
    if {![matchattr $hand $sl_lockflags]} {
      putidx $idx "You're not global +$sl_lockflags." ; return 0
    }
    set locklist ""
    foreach chan [channels] {
      if {[botisop $chan]} {
        sl_quicklock $chan
        lappend locklist $chan
      }
    }
    putidx $idx "Locked [join $locklist ", "]"
  } else {
    if {$chan == ""} {
      set chan [lindex [console $idx] 0]
    }
    if {![validchan $chan]} {
      putidx $idx "No such channel." ; return 0
    } elseif {![matchattr $hand $sl_lockflags|$sl_lockflags $chan]} {
      putidx $idx "You're not +$sl_lockflags on $chan." ; return 0
    } elseif {![botonchan $chan]} {
      putidx $idx "I'm not on $chan" ; return 0
    } elseif {![botisop $chan]} {
      putidx $idx "I'm not opped on $chan" ; return 0
    }
    sl_quicklock $chan
    putidx $idx "Locked $chan"
  }
  return 0
}

proc sl_dccuc {hand idx arg} {
  global sl_lockflags
  putcmdlog "#$hand# unlock $arg"
  set chan [lindex [split $arg] 0]
  if {$chan == "-all"} {
    if {![matchattr $hand $sl_lockflags]} {
      putidx $idx "You're not global +$sl_lockflags." ; return 0
    }
    set locklist ""
    foreach chan [channels] {
      if {[botisop $chan]} {
        putserv "MODE $chan -mi"
        lappend locklist $chan
      }
    }
    putidx $idx "Unlocked [join $locklist ", "]"
  } else {
    if {$chan == ""} {
      set chan [lindex [console $idx] 0]
    }
    if {![validchan $chan]} {
      putidx $idx "No such channel." ; return 0
    } elseif {![matchattr $hand $sl_lockflags|$sl_lockflags $chan]} {
      putidx $idx "You're not +$sl_lockflags on $chan." ; return 0
    } elseif {![botonchan $chan]} {
      putidx $idx "I'm not on $chan" ; return 0
    } elseif {![botisop $chan]} {
      putidx $idx "I'm not opped on $chan" ; return 0
    }
    putserv "MODE $chan -mi"
    putidx $idx "Unlocked $chan"
  }
  return 0
}

proc sl_quicklock {chan} {
  global numversion
  if {$numversion < 1050000} {
    putquick "MODE $chan +mi"
  } else {
    putquick "MODE $chan +mi" -next
  }
}

proc sl_dcc {hand idx arg} {
  global sl_avflood sl_ban sl_banmax sl_bcflood sl_boban sl_boflood sl_bmflood sl_bxsimul sl_bfmaxbans sl_ccflood sl_detectquits sl_globalban sl_igtime sl_jflood sl_kicks sl_lockcmds sl_lockflags sl_ilocktime sl_mlocktime sl_nkflood sl_note sl_shortlock sl_tsunami sl_txflood
  putcmdlog "#$hand# sentinel $arg"
  putidx $idx "This bot is protected by sentinel.tcl by slennox"
  putidx $idx "Current settings"
  if {[lsearch -exact $sl_bcflood 0] != -1} {
    putidx $idx "- Bot CTCP flood:           Off"
  } else {
    putidx $idx "- Bot CTCP flood:           [lindex $sl_bcflood 0] in [lindex $sl_bcflood 1] secs"
  }
  if {[lsearch -exact $sl_bmflood 0] != -1} {
    putidx $idx "- Bot MSG flood:            Off"
  } else {
    putidx $idx "- Bot MSG flood:            [lindex $sl_bmflood 0] in [lindex $sl_bmflood 1] secs"
  }
  if {[lsearch -exact $sl_ccflood 0] != -1} {
    putidx $idx "- Channel CTCP flood:       Off"
  } else {
    putidx $idx "- Channel CTCP flood:       [lindex $sl_ccflood 0] in [lindex $sl_ccflood 1] secs"
  }
  if {[lsearch -exact $sl_avflood 0] != -1} {
    putidx $idx "- Channel AVALANCHE flood:  Off"
  } else {
    putidx $idx "- Channel AVALANCHE flood:  [lindex $sl_avflood 0] in [lindex $sl_avflood 1] secs"
  }
  if {[lsearch -exact $sl_avflood 0] != -1 || !$sl_tsunami} {
    putidx $idx "- Channel TSUNAMI flood:    Off"
  } else {
    putidx $idx "- Channel TSUNAMI flood:    [lindex $sl_avflood 0] in [lindex $sl_avflood 1] secs ($sl_tsunami ctrl codes / line)"
  }
  if {[lsearch -exact $sl_txflood 0] != -1} {
    putidx $idx "- Channel TEXT flood:       Off"
  } else {
    putidx $idx "- Channel TEXT flood:       [lindex $sl_txflood 0] in [lindex $sl_txflood 1] secs"
  }
  if {[lsearch -exact $sl_boflood 0] != -1} {
    putidx $idx "- Channel BOGUS flood:      Off"
  } else {
    putidx $idx "- Channel BOGUS flood:      [lindex $sl_boflood 0] in [lindex $sl_boflood 1] secs"
  }
  if {$sl_detectquits} {
    set detectquits "quit detection ON"
  } else {
    set detectquits "quit detection OFF"
  }
  if {[lsearch -exact $sl_jflood 0] != -1} {
    putidx $idx "- Channel JOIN-PART flood:  Off"
  } else {
    putidx $idx "- Channel JOIN-PART flood:  [lindex $sl_jflood 0] in [lindex $sl_jflood 1] secs ($detectquits)"
  }
  if {[lsearch -exact $sl_nkflood 0] != -1} {
    putidx $idx "- Channel NICK flood:       Off"
  } else {
    putidx $idx "- Channel NICK flood:       [lindex $sl_nkflood 0] in [lindex $sl_nkflood 1] secs"
  }
  if {!$sl_ilocktime} {
    putidx $idx "- Channel +i locktime:      Indefinite"
  } else {
    putidx $idx "- Channel +i locktime:      $sl_ilocktime secs"
  }
  if {!$sl_mlocktime} {
    putidx $idx "- Channel +m locktime:      Indefinite"
  } else {
    putidx $idx "- Channel +m locktime:      $sl_mlocktime secs"
  }
  if {$sl_shortlock && $sl_ban} {
    putidx $idx "- Small flood short lock:   Active"
  } else {
    putidx $idx "- Small flood short lock:   Inactive"
  }
  if {$sl_ban && $sl_ban < 120} {
    putidx $idx "- Channel flood bans:       $sl_ban mins"
  } elseif {$sl_ban >= 120} {
    putidx $idx "- Channel flood bans:       [expr $sl_ban / 60] hrs"
  } else {
    putidx $idx "- Channel flood bans:       Disabled"
  }
  if {!$sl_boban || [lsearch -exact $sl_boflood 0] != -1} {
    putidx $idx "- Bogus username bans:      Disabled"
  } elseif {$sl_boban > 0 && $sl_boban < 120} {
    putidx $idx "- Bogus username bans:      $sl_boban mins"
  } elseif {$sl_boban >= 120} {
    putidx $idx "- Bogus username bans:      [expr $sl_boban / 60] hrs"
  }
  if {$sl_ban || [lsearch -exact $sl_boflood 0] == -1} {
    if {$sl_globalban} {
      putidx $idx "- Ban type:                 Global [sl_masktype nick@host.domain]"
    } else {
      putidx $idx "- Ban type:                 Channel-specific [sl_masktype nick@host.domain]"
    }
  }
  if {$sl_ban || [lsearch -exact $sl_boflood 0] == -1} {
    putidx $idx "- Maximum bans:             $sl_banmax"
  }
  if {$sl_igtime > 0 && $sl_igtime < 120} {
    putidx $idx "- Flooder ignores:          $sl_igtime mins"
  } elseif {$sl_igtime >= 120} {
    putidx $idx "- Flooder ignores:          [expr $sl_igtime / 60] hrs"
  } else {
    putidx $idx "- Flooder ignores:          Permanent"
  }
  if {$sl_ban} {
    putidx $idx "- Kicks per line:           $sl_kicks"
  }
  if {!$sl_bfmaxbans} {
    putidx $idx "- Maximum channel bans:     Disabled"
  } else {
    putidx $idx "- Maximum channel bans:     $sl_bfmaxbans"
  }
  if {$sl_note != ""} {
    putidx $idx "- Flood notification:       Notifying [join $sl_note ", "]"
  } else {
    putidx $idx "- Flood notification:       Off"
  }
  if {!$sl_lockcmds} {
    putidx $idx "- Public lc/uc commands:    Disabled"
  } elseif {$sl_lockcmds == 1} {
    putidx $idx "- Public lc/uc commands:    Enabled (+$sl_lockflags users, ops not required)"
  } elseif {$sl_lockcmds == 2} {
    putidx $idx "- Public lc/uc commands:    Enabled (+$sl_lockflags users, ops required)"
  }
  if {$sl_bxsimul} {
    putidx $idx "- BitchX simulation:        On"
  } elseif {!$sl_bxsimul} {
    putidx $idx "- BitchX simulation:        Off"
  }
  return 0
}

if {$sl_bxsimul} {
  bind raw - 001 sl_bxserverjoin
  if {![info exists sl_bxonestack]} {
    set sl_bxonestack 0
  }
  if {![info exists sl_bxversion]} {
    set sl_bxversion [lindex {75p1+ 75p3+} [rand 2]]
  }
  set sl_bxsystem "*IX" ; set sl_bxwhoami $username ; set sl_bxmachine ""
  catch {set sl_bxsystem [exec uname -s -r]}
  catch {set sl_bxwhoami [exec id -un]}
  catch {set sl_bxmachine [exec uname -n]}
  set sl_bxjointime [unixtime]
  proc sl_bxserverjoin {from keyword arg} {
    global sl_bxjointime sl_bxisaway
    set sl_bxjointime [unixtime] ; set sl_bxisaway 0
    return 0
  }
  proc sl_bxaway {} {
    global sl_bxjointime sl_bxisaway
    if {!$sl_bxisaway} {
      puthelp "AWAY :is away: (Auto-Away after 10 mins) \[\002BX\002-MsgLog [lindex {On Off} [rand 2]]\]"
      set sl_bxisaway 1
    } else {
      puthelp "AWAY"
      set sl_bxisaway 0 ; set sl_bxjointime [unixtime]
    }
    if {![string match *sl_bxaway* [timers]]} {
      timer [expr [rand 300] + 10] sl_bxaway
    }
    return 0
  }
  if {![info exists sl_bxisaway]} {
    set sl_bxisaway 0
  }
  if {![string match *sl_bxaway* [timers]]} {
    timer [expr [rand 300] + 10] sl_bxaway
  }
}

proc sl_setarray {chan} {
  global sl_avbanhost sl_avbannick sl_avqueue sl_bfull sl_bobanhost sl_bobannick sl_boqueue sl_ccbanhost sl_ccbannick sl_ccqueue sl_flooded sl_jbanhost sl_jbannick sl_jqueue sl_locked sl_nkbanhost sl_nkflooding sl_nkqueue sl_pbanhost sl_pbannick sl_pqueue sl_txqueue sl_unlocked
  set chan [string tolower $chan]
  sl_killutimer "incr sl_*queue($chan) -1"
  sl_killutimer "sl_*banqueue $chan"
  sl_killutimer "sl_*queuereset $chan"
  set sl_flooded($chan) 0 ; set sl_locked($chan) 0 ; set sl_unlocked($chan) [unixtime]
  set sl_nkflooding($chan) [unixtime]
  set sl_ccqueue($chan) 0 ; set sl_ccbanhost($chan) "" ; set sl_ccbannick($chan) ""
  set sl_avqueue($chan) 0 ; set sl_avbanhost($chan) "" ; set sl_avbannick($chan) ""
  set sl_txqueue($chan) 0
  set sl_nkqueue($chan) 0 ; set sl_nkbanhost($chan) ""
  set sl_boqueue($chan) 0 ; set sl_bobanhost($chan) "" ; set sl_bobannick($chan) ""
  set sl_jqueue($chan) 0 ; set sl_jbanhost($chan) "" ; set sl_jbannick($chan) ""
  set sl_pqueue($chan) 0 ; set sl_pbanhost($chan) "" ; set sl_pbannick($chan) ""
  set sl_bfull($chan) 0
  return 0
}

proc sl_unsetarray {chan} {
  global sl_avbanhost sl_avbannick sl_avqueue sl_bfull sl_bobanhost sl_bobannick sl_boqueue sl_ccbanhost sl_ccbannick sl_ccqueue sl_flooded sl_jbanhost sl_jbannick sl_jqueue sl_locked sl_nkbanhost sl_nkflooding sl_nkqueue sl_pbanhost sl_pbannick sl_pqueue sl_txqueue sl_unlocked
  set chan [string tolower $chan]
  if {![validchan $chan] && [info exists sl_flooded($chan)]} {
    unset sl_flooded($chan) sl_locked($chan) sl_unlocked($chan) sl_nkflooding($chan) sl_ccqueue($chan) sl_ccbanhost($chan) sl_ccbannick($chan) sl_avqueue($chan) sl_avbanhost($chan) sl_avbannick($chan) sl_txqueue($chan) sl_nkqueue($chan) sl_nkbanhost($chan) sl_boqueue($chan) sl_bobanhost($chan) sl_bobannick($chan) sl_jqueue($chan) sl_jbanhost($chan) sl_jbannick($chan) sl_pqueue($chan) sl_pbanhost($chan) sl_pbannick($chan) sl_bfull($chan)
  }
  return 0
}

proc sl_settimer {} {
  foreach chan [channels] {
    sl_setarray $chan
  }
  return 0
}

proc sl_killutimer {cmd} {
  set n 0
  regsub -all -- {\[} $cmd {\[} cmd ; regsub -all -- {\]} $cmd {\]} cmd
  foreach tmr [utimers] {
    if {[string match $cmd [join [lindex $tmr 1]]]} {
      killutimer [lindex $tmr 2]
      incr n
    }
  }
  return $n
}

proc sl_masktype {uhost} {
  global sl_masktype
  switch -exact -- $sl_masktype {
    0 {return *!*[string range $uhost [string first @ $uhost] end]}
    1 {return *!*$uhost}
    2 {return *!*[lindex [split [maskhost $uhost] "!"] 1]}
  }
  return
}

if {![info exists sl_unlocked] && ![string match *sl_settimer* [utimers]]} {
  utimer 3 sl_settimer
}

if {![info exists sl_bflooded]} {
  set sl_bflooded 0
}
if {![info exists sl_bcqueue]} {
  set sl_bcqueue 0
}
if {![info exists sl_bmqueue]} {
  set sl_bmqueue 0
}
if {![info exists sl_nickkick]} {
  set sl_nickkick 0
}

set sl_bcflood [split $sl_bcflood :] ; set sl_bmflood [split $sl_bmflood :]
set sl_ccflood [split $sl_ccflood :] ; set sl_avflood [split $sl_avflood :]
set sl_txflood [split $sl_txflood :] ; set sl_boflood [split $sl_boflood :]
set sl_jflood [split $sl_jflood :] ; set sl_nkflood [split $sl_nkflood :]
set sl_note [split $sl_note]

if {$sl_ilocktime > 0 && $sl_ilocktime < 30} {
  set sl_ilocktime 30
}
if {$sl_mlocktime > 0 && $sl_mlocktime < 30} {
  set sl_mlocktime 30
}

set trigger-on-ignore 0
if {!${kick-method}} {
  set sl_kicks 8
} else {
  set sl_kicks ${kick-method}
}

if {$numversion <= 1040400} {
  if {$numversion >= 1032100} {
    set kick-bogus 0
  }
  if {$numversion >= 1032400} {
    set ban-bogus 0
  }
}
if {$numversion >= 1032400} {
  set kick-fun 0 ; set ban-fun 0
}
if {$numversion >= 1032500} {
  set ctcp-mode 0
}

if {![string match *sl_txqueuereset* [utimers]] && [lsearch -exact $sl_txflood 0] == -1} {
  utimer [lindex $sl_txflood 1] sl_txqueuereset
}

bind pub $sl_lockflags|$sl_lockflags lc sl_lc
bind pub $sl_lockflags|$sl_lockflags uc sl_uc
bind dcc $sl_lockflags|$sl_lockflags lock sl_dcclc
bind dcc $sl_lockflags|$sl_lockflags unlock sl_dccuc
if {!$sl_lockcmds} {
  unbind pub $sl_lockflags|$sl_lockflags lc sl_lc
  unbind pub $sl_lockflags|$sl_lockflags uc sl_uc
  rename sl_lc ""
  rename sl_uc ""
}
bind dcc m|m sentinel sl_dcc
bind raw - NOTICE sl_avflood
bind raw - PRIVMSG sl_avflood
if {[lsearch -exact $sl_avflood 0] != -1 && [lsearch -exact $sl_txflood 0] != -1} {
  unbind raw - NOTICE sl_avflood
  unbind raw - PRIVMSG sl_avflood
  rename sl_avflood ""
}
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
if {[lsearch -exact $sl_bmflood 0] != -1} {
  unbind msgm - * sl_bmflood
  rename sl_bmflood ""
}
bind nick - * sl_nkflood
if {[lsearch -exact $sl_nkflood 0] != -1} {
  unbind nick - * sl_nkflood
  rename sl_nkflood ""
}
bind join - * sl_jflood
bind part - * sl_pflood
bind sign - * sl_pflood
if {![info exists sl_detectquits]} {
  set sl_detectquits 0
}
if {!$sl_detectquits} {
  unbind sign - * sl_pflood
}
bind kick - * sl_pfloodk
bind flud - * sl_flud
bind mode - * sl_mode

putlog "Loaded sentinel.tcl v2.70 by slennox"

return
