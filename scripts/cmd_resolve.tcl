#
# cmd_resolve.tcl
#  written by Jeff Fisher (guppy@eggheads.org)
#
# This script adds the commands '.resolve' and '.dns' which can be used to
# lookup hostnames or ip addresses in the partyline without causing the bot
# to block while doing so thanks to the dns module.
#
# updates
# -------
#  15Apr2003: fixed a logging bug and stop using regexp incorrectly
#  05Nov2000: fixed a nasty security hole, .resolve [die] <grin>
#  04Nov2000: first version
#
# $Id: cmd_resolve.tcl,v 1.4 2003/04/16 01:03:04 guppy Exp $

bind dcc -|- resolve resolve_cmd
bind dcc -|- dns resolve_cmd

proc resolve_cmd {hand idx arg} {
  global lastbind
  if {[scan $arg "%s" hostip] != 1} {
    putidx $idx "Usage: $lastbind <host or ip>"
  } else {
    putidx $idx "Looking up $hostip ..."
    set hostip [split $hostip]
    dnslookup $hostip resolve_callback $idx $hostip $lastbind
  }
  return 0
}

proc resolve_callback {ip host status idx hostip cmd} {
  if {![valididx $idx]} {
    return 0
  } elseif {!$status} {
    putidx $idx "Unable to resolve $hostip"
  } elseif {[string tolower $ip] == [string tolower $hostip]} {
    putidx $idx "Resolved $ip to $host"
  } else {
    putidx $idx "Resolved $host to $ip"
  }
  putcmdlog "#[idx2hand $idx]# $cmd $hostip"
  return 0
}

loadhelp cmd_resolve.help

putlog "Loaded cmd_resolve.tcl successfully."
