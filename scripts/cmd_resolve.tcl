#
# cmd_resolve.tcl
#  written by Jeff Fisher (guppy@eggheads.org)
#
# This script adds the command '.resolve' which can be used to lookup hostnames
# or ip addresses in the partyline without causing the bot to block while doing
# so thanks to the dns module.
#
# updates
# -------
#  05Nov2000: fixed a nasty security hole, .resolve [die] <grin>
#  04Nov2000: first version
#
# $Id: cmd_resolve.tcl,v 1.1 2000/11/05 21:36:47 fabian Exp $

bind dcc -|- resolve resolve_cmd
bind dcc -|- dns resolve_cmd

proc resolve_cmd {hand idx arg} {
  if {[scan $arg "%s" hostip] != 1} {
    global lastbind
    putidx $idx "Usage: $lastbind <host or ip>"
  } else {
    putidx $idx "Looking up $hostip ..."
    set hostip [split $hostip]
    dnslookup $hostip resolve_callback $idx $hostip
  }
  return 0
}

proc resolve_callback {ip host status idx hostip} {
  if {![valididx $idx]} {
    return 0
  } elseif {!$status} {
    putidx $idx "Unable to resolve $hostip"
  } elseif {[regexp -nocase -- $ip $hostip]} {
    putidx $idx "Resolved $ip to $host"
  } else {
    putidx $idx "Resolved $host to $ip"
  }
  putcmdlog "#[idx2hand $idx]# resolve $hostip"
  return 0
}

loadhelp cmd_resolve.help

putlog " * Loaded [file tail [info script]] successfully."

