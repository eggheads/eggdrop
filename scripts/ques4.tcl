#
# quesedilla, by robey
# v1 -- 20aug95
# v2 -- 2oct95   [improved it]
# v3 -- 17mar96  [fixed it up for 1.0 multi-channel]
# v4 __ 3nov97  [Fixed it up for 1.3.0 version bots] by TG
# v4.00001 nov97 [blurgh]
# 
# this will create an html file every so often (the default is once every
# 5 minutes).  the html file will have a table showing the people currently
# on the channel, their user@hosts, who's an op, and who's idle.  it
# uses a table which some browsers (and pseudo-browsers like lynx) can't
# see, but it can optionally make a second page which will support these
# archaic browsers.  browsers supporting push-pull will receive the updated
# page automatically periodically.
#
# if you have a "url" field defined for a user, their nickname in the
# table will be a link pointing there.  otherwise it checks the info
# line and comment field to see if they start with "http://" -- if so,
# that link will be used.  as a last resort, it will make a "mailto:"
# link if an email address is recorded for the user.
#
# feel free to modify and play with this.  the original was written in
# 15 minutes, then at various times i fixed bugs and added features.
# softlord helped me make the design look a little nicer. :)  if you make
# any nifty improvements, please let me know.
#                                       robey@netcom.com


## this line makes sure other scripts won't interfere
catch {unset webfile}

## for each channel you want a webfile for, do this:
set webfile(#turtle) "/home/robey/public_html/turtle.html"
set webfile(#gloom) "/home/robey/public_html/gloom.html"

## define these for channels you want alternate (lynx-friendly) web pages
## for:  (the 'lynxfilerf' needs to be relative to your html directory)
set lynxfile(#turtle) "/home/robey/public_html/turtle-lynx.html"
set lynxfilerf(#turtle) "turtle-lynx.html"

## how often should these html files get updated?
## (1 means once every minute, 5 means once every 5 minutes, etc)
set web_update 5

## this will help people figure out what timezone you're in :)
set webtz [clock format 0 -format %Z]
## if that line breaks, you're using an older Tcl library and will have to
## set it manually like so:
#set webtz "PST"

proc do_ques {} {
  global webfile lynxfile web_update botnick webtz lynxfilerf
  foreach chan [array names webfile] {
    if {[lsearch -exact [string tolower [channels]] [string tolower $chan]] == -1} {continue}
    set fd [open $webfile($chan) w]
    if {[info exists lynxfile($chan)]} {
      set fdl [open $lynxfile($chan) w]
    } else {
      set fdl [open "/dev/null" w]
    }
    puts $fd "<meta http-equiv=\"Refresh\" content=[expr $web_update *60]>"
    if {![onchan $botnick $chan]} {
      puts $fd "<html><head><title>People on $chan right now</title></head>"
      puts $fd "<body><h1>Oops!</h1><p>"
      puts $fd "I'm not on $chan right now for some reason.<br>"
      puts $fd "IRC isn't a very stable place these days... Please try again"
      puts $fd "later!<p></body></html>"
      puts $fdl "<html><head><title>People on $chan right now</title></head>"
      puts $fdl "<body><h1>Oops!</h1><p>"
      puts $fdl "I'm not on $chan right now for some reason.<br>"
      puts $fdl "IRC isn't a very stable place these days... Please try again"
      puts $fdl "later!<p></body></html>"
      close $fd
      close $fdl
    } {
      puts $fd "<html><head><title>People on $chan right now</title></head>"
      puts $fd "<body><center><h1>$chan</h1>"
      puts $fdl "<html><head><title>People on $chan right now</title></head>"
      puts $fdl "<body><h1>$chan</h1>"
      if {[info exists lynxfile($chan)]} {
        puts $fd "<p>If this page looks screwy on your browser, try"
        puts $fd "<a href=\"$lynxfilerf($chan)\">this page</a>.<p>"
      }
      puts $fd "<table border=1 cellpadding=4>"
      puts $fd "<caption>People on $chan as of [date] [time] $webtz</caption>"
      puts $fd "<th align=left>Nickname<th align=left>Status"
      puts $fd "<th align=left>User@Host"
      puts $fdl "<em>People on $chan as of [date] [time] $webtz</em><pre>"
      puts $fdl "Nickname  Status           User@Host"
      foreach i [chanlist $chan] {
        puts $fd "<tr>"
        if {[isop $i $chan]} { set chop "op"} { set chop "" }
        if {[getchanidle $i $chan] > 10} {
          if {$chop == ""} { set chop "idle" } { set chop "${chop}, idle" }
        }
        set handle [finduser $i![getchanhost $i $chan]]
        if {[onchansplit $i $chan]} {
          if {$chop == ""} { 
            set chop "<strong>(split)</strong>"
          } {
            set chop "${chop} <strong>(split)</strong>"
          }
        }
        set url [getuser $handle xtra url]
        set info [getuser $handle info]
        set comment [getuser $handle comment]
        if {"" == $chop} { set chop "-" }
        set link ""
        if {"" != $url} {
          set link "<a href=\"$url\">"
        }
        if {("" == $link) && 
            ([string compare [string range $comment 0 6] "http://"] == 0)} {
          set link "<a href=\"[lindex [split $comment] 0]\">"
        }
        if {("" == $link) &&
            ([string compare [string range $info 0 6] "http://"] == 0)} {
          set link "<a href=\"[lindex [split $info] 0]\">"
        }
        if {"" == $link} {
          set email [getuser $handle xtra email]
          if {("" != $email) && ([string first @ $email] > 0)} {
            set link "<a href=\"mailto:$email\">"
          }
        }
        if {"" != $link} { set elink "</a>" } { set elink "" }
        puts $fd "<td align=left>${link}${i}${elink}</td>"
        puts $fd "<td align=left>${chop}</td>"
        set chost [getchanhost $i $chan]
        if {[string compare [string tolower $i] [string tolower $botnick]] == 0} {
          set chost "<em>This is me, the channel bot.</em>"
          set info ""
        } {
          if {[matchattr $handle b]} {
            set chost "<em>This is another channel bot.</em>"
            set info ""
          }
        }
        puts $fd "<td align=left>${chost}</td>"
        puts $fdl "${link}[format %-9s $i]${elink} [format %-16s $chop] $chost"
        if {"X" != "X$info"} {
          puts $fd "<tr><td></td><td colspan=2><strong>Info:</strong> $info</td>"
          puts $fdl "   <strong>Info:</strong> $info"
        }
      }
      puts $fd "</table></center>"
      puts $fdl "</pre>"
      puts $fd "<p><hr><address>Created by quesedilla v3 via"
      puts $fd "<a href=\"http://www.calweb.com/~xerxes/eggdrop/\">"
      puts $fd "eggdrop</a></address>"
      puts $fdl "<p><hr><address>Created by quesedilla v3 via"
      puts $fdl "<a href=\"http://www.calweb.com/~xerxes/eggdrop/\">"
      puts $fdl "eggdrop</a></address>"
      puts $fd "<address>This page is automatically refreshed.</address>"
      puts $fd "</body></html>"
      puts $fdl "</body></html>"
      close $fd
      close $fdl
    }
  }
  timer $web_update do_ques
}

if {![info exists ques_going]} {
  timer $web_update do_ques
  set ques_going 1
}
set xxchans ""
foreach i [array names webfile] {
  set xxchans "$xxchans $i"
  if {![info exists lynxfile($i)]} {
    set xxchans "$xxchans (no lynx)"
  }
  set xxchans "${xxchans},"
}
set xxchans [string trimright $xxchans ","]
putlog "Quesedilla v3:$xxchans"
unset xxchans
