###############################################################################
##
##  dccwhois.tcl - Enhanced '.whois' dcc command for Eggdrop
##  Copyright (C) 2009  Tothwolf <tothwolf@techmonkeys.org>
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
###############################################################################
##
## $Id: dccwhois.tcl,v 1.1 2009/01/22 03:12:45 tothwolf Exp $
##
###############################################################################
##
## Description:
##
## This script enhances Eggdrop's built-in dcc '.whois' command to allow all
## users to '.whois' their own handle.
##
## Users without the correct flags who attempt to '.whois' other users will
## instead see the message: "You do not have access to whois handles other
## than your own."
##
## To load this script, add a source command to your bot's config file:
##
##   source scripts/dccwhois.tcl
##
## This script stores and checks against the flags that are used for
## Eggdrop's built-in dcc '.whois' command at load time. If you wish to use
## flags other than the default "to|o", etc, you should unbind and rebind
## the built-in '.whois' command in your bot's config file before loading
## this script.
##
## Example of how to rebind Eggdrop's built-in '.whois' command:
##
##   unbind dcc to|o whois *dcc:whois
##   bind dcc to|m whois *dcc:whois
##
## Note: if you modify the default flags for '.whois' you may also wish to
## modify the defaults for '.match'.
##
###############################################################################
##
##  This script has no settings and does not require any configuration.
##  You should not need to edit anything below.
##
###############################################################################


# This script should not be used with Eggdrop versions 1.6.16 - 1.6.19.
catch {set numversion}
if {([info exists numversion]) &&
    ($numversion >= 1061600) && ($numversion <= 1061900)} then {
  putlog "Error: dccwhois.tcl is not compatible with Eggdrop version [lindex $version 0]. Please upgrade to 1.6.20 or later."
  return
}


#
# dcc:whois --
#
# Wrapper proc command for Eggdrop's built-in *dcc:whois
#
# Arguments:
#   hand - handle of user who used this command
#   idx - dcc idx of user who used this command
#   arg - arguments passed to this command
#
# Results:
#   Calls Eggdrop's built-in *dcc:whois with the given arguments if user has
#   access, otherwise tells the user they don't have access.
#
proc dcc:whois {hand idx arg} {
  global dccwhois_flags

  set arg [split [string trimright $arg]]
  set who [lindex $arg 0]

  # Did user gave a handle other than their own?
  if {([string compare "" $who]) &&
      ([string compare [string toupper $hand] [string toupper $who]])} then {

    # Get user's current .console channel; check the same way Eggdrop does.
    set chan [lindex [console $idx] 0]

    # User isn't allowed to '.whois' handles other than their own.
    if {![matchattr $hand $dccwhois_flags $chan]} then {
      putdcc $idx "You do not have access to whois handles other than your own."

      return 0
    }
  }

  # Call built-in whois command.
  *dcc:whois $hand $idx $arg

  # Return 0 so we don't log command twice.
  return 0
}


#
# init_dccwhois --
#
# Initialize dccwhois script-specific code when script is loaded
#
# Arguments:
#   (none)
#
# Results:
#   Set up command bindings and store command access flags.
#
proc init_dccwhois {} {
  global dccwhois_flags

  putlog "Loading dccwhois.tcl..."

  # Sanity check...
  if {[array exists dccwhois_flags]} then {
    array unset dccwhois_flags
  }

  # Search binds for built-in '*dcc:whois' and loop over each bind in list
  foreach bind [binds "\\*dcc:whois"] {

    # dcc to|o whois 0 *dcc:whois
    foreach {type flags name count command} $bind {break}

    # We only want to unbind dcc '.whois'
    if {[string compare $name "whois"]} then {
      continue
    }

    # Store $flags so we can reuse them later
    set dccwhois_flags $flags

    # Unbind built-in *dcc:whois
    unbind $type $flags $name $command
  }

  # Make sure $dccwhois_flags exists and isn't empty,
  # otherwise set to Eggdrop's default "to|o"
  if {(![info exists dccwhois_flags]) ||
      (![string compare "" $dccwhois_flags])} then {
    set dccwhois_flags "to|o"
  }

  # Add bind for dcc:whois wrapper proc
  bind dcc -|- whois dcc:whois

  putlog "Loaded dccwhois.tcl"

  return
}

init_dccwhois
