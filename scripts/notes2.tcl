#
# notes2.tcl - v2.1.1 - released by MHT <mht@mygale.org>
#                     - a bind apart script from #TSF
#                     - for eggdrop 1.3.15+
#
# $Id: notes2.tcl,v 1.6 2001/11/05 04:08:28 guppy Exp $
#
####
#
# history:
# --------
# 2.0.0 - first release for 1.3.14+mht series
#         (get notesat2.tcl for 1.1.5 series)
#
# 2.0.2 - Message bug corrected: "erased <m> notes; <n> left." is better.
#       - Corrected weird switch tcl syntax, bug found by Islandic.
#         It's so different from C (I hate tcl!).
#       - Desactivated message "I don't know you", boring me !
#       - No more logs for notes-indexing on join :-)
#
# 2.0.3 - Corrected invalid idx bug, if user quits before receiving
#         his notes check.
#
# 2.1.0 - Improved protocol to avoid idx mistake for multiple connected users.
#         Backward compatibility is kept, but price is that idx mistake occurs
#         if a multiple connected user quits before receiving notes check.
#         Generally never happens, except in case of 'Chriphil's syndrome' ;-p
#       - Added missing 'You don't have that many messages.'
#
# 2.1.1 - fixed a couple of small bugs pertaining to $nick being used instead of
#         $botnet-nick (found by takeda, fixed by guppy)
#
####
# Check your notes on every shared bot of the hub.
#
# .notes [bot|all] index
# .notes [bot|all] read  <#|all>
# .notes [bot|all] erase <#|all>
#
# # may be numbers and/or intervals separated by ;
# ex: .notes erase 2-4;8;16-
#     .notes noBOTy read all
#


########
unbind dcc  - notes        *dcc:notes
bind   dcc  - notes        *dcc:notes2
bind   chon - *            *chon:notes2
bind   bot  - notes2:      *bot:notes2
bind   bot  - notes2reply: *bot:notes2reply

########
proc n2_notesindex {bot handle idx} {
    global nick botnet-nick
    switch "([notes $handle])" {
	"(-2)" { putbot $bot "notes2reply: $handle Notefile failure. $idx" }
	#"-1" { putbot $bot "notes2reply: $handle I don't know you. $idx" }
	"(-1)" { return 0 }
	"(0)"  { putbot $bot "notes2reply: $handle You have no messages. $idx" }
	default {
	    putbot $bot "notes2reply: $handle ### You have the following notes waiting: $idx"
	    set index 0
	    foreach note [notes $handle "-"] {
		if {($note != 0)} {
		    incr index
		    set sender [lindex $note 0]
		    set date [strftime "%b %d %H:%M" [lindex $note 1]]
		    putbot $bot "notes2reply: $handle %$index. $sender ($date) $idx"
		}
	    }
	    putbot $bot "notes2reply: $handle ### Use '.notes ${botnet-nick} read' to read them. $idx"
	}
    }
    return 1
}

########
proc n2_notesread {bot handle idx numlist} {
    if {($numlist == "")} { set numlist "-" }
    switch "([notes $handle])" {
        "(-2)" { putbot $bot "notes2reply: $handle Notefile failure. $idx" }
	#"(-1)" { putbot $bot "notes2reply: $handle I don't know you. $idx" }
	"(-1)" { return 0 }
        "(0)"  { putbot $bot "notes2reply: $handle You have no messages. $idx" }
	default {
	    set count 0
	    set list [listnotes $handle $numlist]
	    foreach note [notes $handle $numlist] {
		if {($note != 0)} {
		    set index [lindex $list $count]
		    set sender [lindex $note 0]
		    set date [strftime "%b %d %H:%M" [lindex $note 1]]
		    set msg [lrange $note 2 end]
		    incr count
		    putbot $bot "notes2reply: $handle $index. $sender ($date): $msg $idx"
		} else {
		    putbot $bot "notes2reply: $handle You don't have that many messages. $idx"
		}
	    }
	}
    }
    return 1
}

########
proc n2_noteserase {bot handle idx numlist} {
    switch [notes $handle] {
	"(-2)" { putbot $bot "notes2reply: $handle Notefile failure. $idx" }
	#"(-1)" { putbot $bot "notes2reply: $handle I don't know you. $idx" }
	"(-1)" { return 0 }
	"(0)"  { putbot $bot "notes2reply: $handle You have no messages. $idx" }
	default {
	    set erased [erasenotes $handle $numlist]
	    set remaining [notes $handle]
	    if {($remaining == 0) && ($erased == 0)} {
		putbot $bot "notes2reply: $handle You have no messages. $idx"
	    } elseif {($remaining == 0)} {
		putbot $bot "notes2reply: $handle Erased all notes. $idx"
	    } elseif {($erased == 0)} {
		putbot $bot "notes2reply: $handle You don't have that many messages. $idx"
	    } elseif {($erased == 1)} {
		putbot $bot "notes2reply: $handle Erased 1 note, $remaining left. $idx"
	    } else {
		putbot $bot "notes2reply: $handle Erased $erased notes, $remaining left. $idx"
	    }
	}
    }
    return 1
}

########
proc *bot:notes2 {handle idx arg} {
    if {(![matchattr $handle s])} {
	return
    }
    set nick [lindex $arg 0]
    set cmd  [lindex $arg 1]
    set num  [lindex $arg 2]
    set idx  [lindex $arg 3]
    if {($num == "") || ($num == "all")} { set num "-" }
    switch $cmd {
       "silentindex" { set ret 0; n2_notesindex $handle $nick $idx }
       "index" { set ret [n2_notesindex $handle $nick $idx] }
       "read"  { set ret [n2_notesread $handle $nick $idx $num] }
       "erase" { set ret [n2_noteserase $handle $nick $idx $num] }
       default { set ret 0 }
    }
    if {($num == "-")} { set num "" }
    if {($ret == 1)} { putcmdlog "#$nick@$handle# notes $cmd $num" }
}

########
proc *bot:notes2reply {handle idx arg} {
    # verify that idx is valid (older scripts do not provide idx)
    set idx [lindex $arg end]
    if {([valididx $idx]) && ([idx2hand $idx] == [lindex $arg 0])} {
	set reply [lrange $arg 1 [expr [llength $arg]-2]]
    } else {
	set idx [hand2idx [lindex $arg 0]]
	set reply [lrange $arg 1 end]
    }
    if {($idx == -1)} { return }
    if {([string range $reply 0 0] == "%")} {
	set reply "   [string range $reply 1 end]"
    }
    putidx $idx "($handle) $reply"
}

########
proc *chon:notes2 {handle idx} {
    putallbots "notes2: $handle silentindex $idx"
    return 0
}

########
proc *dcc:notes2 {handle idx arg} {
    global nick botnet-nick
    if {$arg == ""} {
	putidx $idx "Usage: notes \[bot|all\] index"
	putidx $idx "       notes \[bot|all\] read <#|all>"
	putidx $idx "       notes \[bot|all\] erase <#|all>"
	putidx $idx "       # may be numbers and/or intervals separated by ;"
	putidx $idx "       ex: notes erase 2-4;8;16-"
	putidx $idx "           notes ${botnet-nick} read all"
    } else {
	set bot [string tolower [lindex $arg 0]]
	set cmd [string tolower [lindex $arg 1]]
	set numlog [string tolower [lindex $arg 2]]
	set num $numlog
	if {($num == "")} { set num "-" }
	if {($bot != "all") && ([lsearch [string tolower [bots]] $bot] < 0)} {
	    if {($cmd != "index") && ($cmd != "read") && ($cmd != "erase")} {	    
		if {($bot == [string tolower $nick])} {
		    return [*dcc:notes $handle $idx [lrange $arg 1 end]]
		} else {
		    return [*dcc:notes $handle $idx $arg]
		}
	    } else {
		putidx $idx "I don't know any bot by that name."
		return 0
	    }
	} elseif {($cmd != "index") && ($cmd != "read") && ($cmd != "erase")} {
	    putdcc $idx "Function must be one of INDEX, READ, or ERASE."
	} elseif {$bot == "all"} {
	    #*dcc:notes $handle $idx [lrange $arg 1 end]
	    putallbots "notes2: $handle $cmd $num $idx"
	} else {
	    putbot $bot "notes2: $handle $cmd $num $idx"
	}
	putcmdlog "#$handle# notes@$bot $cmd $numlog"
    }
}

########
putlog "Notes 2.1.0 - Released by MHT <mht@mygale.org>"

####
