# userinfo.tcl v1.06 for Eggdrop 1.4.3 and higher
#           Scott G. Taylor -- ButchBub!staylor@mrynet.com
#
# $Id: userinfo.tcl,v 1.5 2001/11/15 06:28:35 guppy Exp $
#
# v1.00      ButchBub     14 July      1997 -Original release.  Based on
#                                            whois.tcl "URL" commands.
# v1.01      Beldin       11 November  1997 -1.3 only version
# v1.02      Kirk         19 June      1998 -extremely small fixes
# v1.03      guppy        17 March     1999 -small fixes again
# v1.04      Ernst        15 June      1999 -fix for egg 1.3.x + TCL 8.0
# v1.05      Dude         14 July      1999 -small cosmetic/typo fixes
#                                           -$lastbind bug work around fix
#                                           -added userinfo_loaded var
#                                           -fixed bug in dcc_chuserinfo proc
#                                           -unbinds removed user fields
#                                           -dcc .showfields command added
#                                           -deletes removed userinfo fields
#                                            from the whois-fields list.
# v1.06      guppy        19 March     2000 -removed lastbind workaround since
#                                            lastbind is fixed in eggdrop1.4.3
# v1.07      TaKeDa       20 August    2001 -now script works also on bots,
#                                            which didn't have server module loaded
#                                           -added new fields PHONE & ICQ
#
# TO USE:  o    Set the desired userinfo field keywords to the
#               `userinfo-fields' line below where indicated.
#          o    Load this script on a 1.1.6 or later Eggdrop bot.
#          o    Begin having users save the desired information.  If you
#               choose to add the default "IRL" field, they just use
#               the IRC command: /MSG <botnick> irl Joe Blow.
#          o    See the new information now appear with the whois command.
#
# This script enhances the `whois' output utilizing the `whois-fields'
# option of eggdrop 1.1-grant and later versions.  It adds the functionality
# of whois.tcl used in pre-1.1-grant versions.
#
# The fields desired to be maintained in the userfile `xtra' information
# should be put in `userinfo-fields'.  This is different than the Eggdrop
# configuration variable `whois-fields' in that this script will add the
# commands to change these fields.   It will also add these desired fields
# to the `whois-fields' itself, so do not define them there as well.  The
# fields added in `userinfo-fields' will be converted to upper case for
# aesthetics in the `whois' command output.
#
# The commands that will be added to the running eggdrop are:
#  (<info> will be the respective userfile field added in `userinfo-fields')
#
#   TYPE    COMMAND         USAGE
#   ======  ==============  ========================================
#   msg      <info>         To change your <info> via /MSG.
#   dcc     .<info>         To change your <info> via DCC.
#   dcc     .ch<info>       To change someone else's <info> via DCC.
#
# Currently supported fields and commands:
#
#   FIELD   USAGE
#   =====   =====================
#   URL     WWW page URL
#   IRL     In Real Life name
#   BF      Boyfriend
#   GF      Girlfriend
#   DOB     Birthday (Date Of Birth)
#   EMAIL   Email address
#   PHONE   Phone number
#   ICQ     ICQ number


################################
# Set your desired fields here #
################################

set userinfo-fields "URL BF GF IRL EMAIL DOB PHONE ICQ"

# This script's identification

set userinfover "Userinfo TCL v1.07"

# This script is NOT for pre-1.4.3 versions.

catch { set numversion }
if {![info exists numversion] || ($numversion < 1040300)} {
  putlog "*** Can't load $userinfover -- At least Eggdrop v1.4.3 required"
    return 0
}

# Make sure we don't bail because whois and/or userinfo-fields aren't set.

if { ![info exists whois-fields]} { set whois-fields "" }
if { ![info exists userinfo-fields]} { set userinfo-fields "" }

# Add only the userinfo-fields not already in the whois-fields list.

foreach field [string tolower ${userinfo-fields}] {
 if { [lsearch -exact [string tolower ${whois-fields}] $field] == -1 } { append whois-fields " " [string toupper $field] }
}

# If olduserinfo-fields doesn't exist, create it.

if { ![info exists olduserinfo-fields] } { set olduserinfo-fields ${userinfo-fields} }

# Delete only the userinfo-fields that have been removed but are still
# listed in the whois-fields list.

foreach field [string tolower ${olduserinfo-fields}] {
 if { [lsearch -exact [string tolower ${whois-fields}] $field] >= 0 && [lsearch -exact [string tolower ${userinfo-fields}] $field] == -1 } {
  set fieldtmp [lsearch -exact [string tolower ${whois-fields}] $field]
  set whois-fields [lreplace ${whois-fields} $fieldtmp $fieldtmp]
 }
}

# If olduserinfo-fields don't equal userinfo-fields, lets run through the
# old list of user fields and compare them with the current list. this way
# any fields that have been removed that were originally in the list will
# have their msg/dcc commands unbinded so you don't have to do a restart.

if {[info commands putserv] == ""} {
 set isservermod 0
} else {
 set isservermod 1
}

if { [string tolower ${olduserinfo-fields}] != [string tolower ${userinfo-fields}] } {
 foreach field [string tolower ${olduserinfo-fields}] {
  if { [lsearch -exact [string tolower ${userinfo-fields}] $field] == -1 } {
   if $isservermod {unbind msg - $field msg_setuserinfo}
   unbind dcc - $field dcc_setuserinfo
   unbind dcc m ch$field dcc_chuserinfo
  }
 }
 set olduserinfo-fields ${userinfo-fields}
}

# Run through the list of user info fields and bind their commands

if { ${userinfo-fields} != "" } {
 foreach field [string tolower ${userinfo-fields}] {
  if $isservermod {bind msg - $field msg_setuserinfo}
  bind dcc - $field dcc_setuserinfo
  bind dcc m ch$field dcc_chuserinfo
 }
}

# This is the `/msg <info>' procedure
if $isservermod {

proc msg_setuserinfo {nick uhost hand arg} {
  global lastbind quiet-reject userinfo-fields
   set userinfo [string toupper $lastbind]
  set arg [cleanarg $arg]
  set ignore 1
  foreach channel [channels] {
    if {[onchan $nick $channel]} {
      set ignore 0
    }
  }
  if {$ignore} {
    return 0
  }
   if {$hand != "*"} {
      if {$arg != ""} {
         if {[string tolower $arg] == "none"} {
            putserv "NOTICE $nick :Removed your $userinfo line."
            setuser $hand XTRA $userinfo ""
         } {
            putserv "NOTICE $nick :Now: $arg"
            setuser $hand XTRA $userinfo "[string range $arg 0 159]"
         }
      } {
         if {[getuser $hand XTRA $userinfo] == ""} {
            putserv "NOTICE $nick :You have no $userinfo set."
         } {
            putserv "NOTICE $nick :Currently: [getuser $hand XTRA $userinfo]"
         }
      }
   } else {
      if {${quiet-reject} != 1} {
        putserv "NOTICE $nick :You must be a registered user to use this feature."
      }
   }
  putcmdlog "($nick!$uhost) !$hand! $userinfo $arg"
  return 0
}

#checking for server module
}

# This is the dcc '.<info>' procedure.

proc dcc_setuserinfo {hand idx arg} {
  global lastbind userinfo-fields
    set userinfo [string toupper $lastbind]
  set arg [cleanarg $arg]
  if {$arg != ""} {
    if {[string tolower $arg] == "none"} {
      putdcc $idx "Removed your $userinfo line."
      setuser $hand XTRA $userinfo ""
    } {
      putdcc $idx "Now: $arg"
      setuser $hand XTRA $userinfo "[string range $arg 0 159]"
    }
  } {
    if {[getuser $hand XTRA $userinfo] == ""} {
      putdcc $idx "You have no $userinfo set."
    } {
      putdcc $idx "Currently: [getuser $hand XTRA $userinfo]"
    }
  }
  putcmdlog "#$hand# [string tolower $userinfo] $arg"
  return 0
}

# This is the DCC `.ch<info>' procedure

proc dcc_chuserinfo {hand idx arg} {
  global lastbind userinfo-fields
   set userinfo [string toupper [string range $lastbind 2 end]]
  set arg [cleanarg $arg]
  if { $arg == "" } {
    putdcc $idx "syntax: .ch[string tolower $userinfo] <who> \[<[string tolower $userinfo]>|NONE\]"
    return 0
  }
  set who [lindex [split $arg] 0]
  if {![validuser $who]} {
    putdcc $idx "$who is not a valid user."
    return 0
  }
  if {[llength [split $arg]] == 1} {
    set info ""
  } {
    set info [lrange [split $arg] 1 end]
  }
  if {$info != ""} {
    if {[string tolower $info] == "none"} {
      putdcc $idx "Removed $who's $userinfo line."
      setuser $who XTRA $userinfo ""
      putcmdlog "$userinfo for $who removed by $hand"
    } {
      putdcc $idx "Now: $info"
      setuser $who XTRA $userinfo "$info"
      putcmdlog "$userinfo for $who set to \"$info\" by $hand"
    }
  } {
    if {[getuser $who XTRA $userinfo] == ""} {
      putdcc $idx "$who has no $userinfo set."
    } {
      putdcc $idx "Currently: [getuser $who XTRA $userinfo]"
    }
  }
  return 0
}

bind dcc m showfields showfields

proc showfields {hand idx arg} {
 global userinfo-fields
 if { ${userinfo-fields} == "" } {
  putdcc $idx "Their is no user info fields set."
  return 0
 }  
 putdcc $idx "Currently: [string toupper ${userinfo-fields}]"
 putcmdlog "#$hand# showfields"
 return 0
}

proc cleanarg {arg} {
  set response ""
  for {set i 0} {$i < [string length $arg]} {incr i} {
    set char [string index $arg $i]
    if {($char != "\12") && ($char != "\15")} {
      append response $char
    }
  }
  return $response
}

# Set userinfo_loaded variable to indicate that the script was successfully
# loaded. this can be used in scripts that make use of the userinfo tcl.

set userinfo_loaded 1

# Announce that we've loaded the script.

putlog "$userinfover loaded (${userinfo-fields})."
putlog "use '.help userinfo' for commands."
