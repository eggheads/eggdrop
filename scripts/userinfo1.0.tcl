#
# userinfo.tcl v1.02 for Eggdrop 1.1.6 and higher
#           Scott G. Taylor -- ButchBub!staylor@mrynet.com
#
# V1.00      ButchBub     14 July      1997  Original release.  Based on
#                       whois.tcl "URL" commands.
# v1.01      Beldin       11 November  1997  1.3 only version
# v1.02      Kirk         19 June      1998  extremely small fixes
# v1.03      guppy        17 March     1999  small fixes again
# v1.04      Ernst        15 June      1999  fix for egg 1.3.x + TCL 8.0
#
# TO USE:  o    Set the desired userinfo field keywords to the
#       `userinfo-fields' line below where indicated.
#      o    Load this script on a 1.1.6 or later Eggdrop bot.
#      o    Begin having users save the desired information.  If you
#       chose to add the default "IRL" field, they just use
#       the IRC command: /MSG <botnick> irl Joe Blow.
#      o    See the new information now appear with the whois command.
#
# This script enhances the `whois' output utilising the `whois-fields'
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
#   TYPE    COMMAND     USAGE
#   ======  ==============  ========================================
#   msg <info>      To change your <info> via /MSG.
#   dcc .<info>     To change your <info> via DCC.
#   dcc .ch<info>   To change someone else's <info> via DCC.
#
# Currently supported fields and commands:
#
#   FIELD   USAGE
#   =====   =====================
#   URL WWW page URL
#   IRL In Real Life name
#   BF  Boyfriend
#   GF  Girlfriend
#   DOB     Birthday (Date Of Birth)
#       EMAIL   Email address

################################
# Set your desired fields here #
################################

set userinfo-fields "URL BF GF IRL EMAIL DOB"

# This script's identification

set userinfover "Userinfo v1.02"

# This script is NOT for pre-1.3.0 versions.

catch { set numversion }
if {![info exists numversion] || ($numversion < 1030000)} {
    if {[string range $version 0 2] != "1.3"} {
    putlog "*** Can't load $userinfover -- At least Eggdrop v1.3.0 required"
    return 0
  }
}

# Make sure we don't bail because whois-fields isn't set

if {![info exists whois-fields]} {
  set whois-fields ""
}

# Add only the user-xtra fields not already in the whois-fields list

foreach f1 [split ${userinfo-fields}] {
  set ffound 0
  foreach f2 [split ${whois-fields}] {
    if {[string tolower $f1] == [string tolower $f2]} {
      set ffound 1
      break
    }
  }
  if {$ffound == 0} {
    append whois-fields " " $f1
  }
}

# Run through the list of user info fields and bind their commands

foreach field [split ${userinfo-fields}] {
  bind msg - $field msg_setuserinfo
  bind dcc - $field dcc_setuserinfo
  bind dcc m ch$field dcc_chuserinfo
}

# This is the `/msg <info>' procedure

proc msg_setuserinfo {nick uhost hand arg} {
  global lastbind quiet-reject

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
            putserv "NOTICE $nick :Now: [getuser $hand XTRA $userinfo]"
         }
      }
   } else {
      if {${quiet-reject} != 1} {
        putserv
          "NOTICE $nick :Ymust be a registered user to use this feature."
      }
   }
   return 1
}

# This is the dcc '.<info>' procedure.

proc dcc_setuserinfo {hand idx arg} {
global lastbind
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
  return 1
}

# This is the DCC `.ch<info>' procedure

proc dcc_chuserinfo {hand idx arg} {
global lastbind
  set userinfo [string toupper [string range $lastbind 2 end]]
  set arg [cleanarg $arg]
  if {[llength [split $arg]] == 0} {
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
    if {[getuser $who $userinfo] == ""} {
      putdcc $idx "$who has no $userinfo set."
    } {
      putdcc $idx "Currently: [getuser $who $userinfo]"
    }
  }
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

# Announce that we've loaded the script.

putlog "$userinfover by ButchBub loaded for: ${userinfo-fields}."
putlog "use '.help userinfo' for comands."
