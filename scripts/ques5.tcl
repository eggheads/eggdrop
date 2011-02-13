#
# ques5.tcl
#
# Copyright (C) 1995 - 1997 Robey Pointer
# Copyright (C) 1999 - 2011 Eggheads Development Team
#
# v1 -- 20aug95
# v2 -- 2oct95   [improved it]
# v3 -- 17mar96  [fixed it up for 1.0 multi-channel]
# v4 -- 3nov97  [Fixed it up for 1.3.0 version bots] by TG
# v4.00001 nov97 [blurgh]
# v5-BETA1 -- 26sep99 by rtc
#
# $Id: ques5.tcl,v 1.18 2011/02/13 14:19:33 simple Exp $
#
# o clean webfile var removal
# o using timezone variable from config file
# o unified options and removed unnecessary ones.
# o convert urls, nicks etc. to HTML before we put them into the page.
# o nice html source indenting
# o replace the old file after the new one has completely written to
#   disk
# o the description still contained robey's address, replaced
#   by the eggheads email.
# o don't link any spaces in the HTML2.0 file
# v5-RC1 -- 29sep99 by rtc
# o info line wasn't converted to HTML.
# o now supports bold, italic and underline text style and colors.
# v5-FINAL -- 04oct99 by rtc
# o style converter now strictly follows HTML standard.
# o Fake color attributes with number > 2^32 don't cause Tcl
#   error anymore.
# o now uses strftime as time and date functions have both been removed
#   in 1.3.29

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
# any nifty improvements, please let us know.
#                                       eggheads@eggheads.org

# this line makes sure other scripts won't interfere
if {[info exists web_file] || [array exists web_file]} {unset web_file}

# You must define each channel you want a webfile for .
# If you want a HTML2.0 file, too, put it's filename separated by
# a colon to the same option, it goes to the same directory.
#set web_file(#turtle) "/home/lamest/public_html/turtle.html:turtle-lynx.html"

# This example demonstrates how to put lynx files into another dir.
#set web_file(#gloom) "/home/lamest/public_html/gloom.html:lynx/gloom.html"

# You can also prevent the HTML2.0 file from being written.
#set web_file(#channel) "/home/lamest/public_html/channel.html"

# You can even let the bot write only a HTML2.0.
#set web_file(#blah) "/home/lamest/public_html/:blah.html"

# how often should these html files get updated?
# (1 means once every minute, 5 means once every 5 minutes, etc)
set web_update 5

# Which characters should be allowed in URLs?
# DO NOT MODIFY unless you really know what you are doing.
# Especially never add '<', '"' and '>'
set web_urlchars "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 :+-/!\$%&()=[]{}#^~*.:,;\\|?_@"

# IRC -> HTML color translation table
set web_color(0) "#FFFFFF"
set web_color(1) "#000000"
set web_color(2) "#00007F"
set web_color(3) "#008F00"
set web_color(4) "#FF0000"
set web_color(5) "#7F0000"
set web_color(6) "#9F009F"
set web_color(7) "#FF7F00"
set web_color(8) "#F0FF00"
set web_color(9) "#00F700"
set web_color(10) "#008F8F"
set web_color(11) "#00F7FF"
set web_color(12) "#0000FF"
set web_color(13) "#FF00FF"
set web_color(14) "#7F7F7F"
set web_color(15) "#CFCFCF"

# IRC -> HTML style translation table
set web_style(\002) "<B> </B>"
set web_style(\003) "<FONT> </FONT>"
set web_style(\026) "<I> </I>"
set web_style(\037) "<U> </U>"

proc getnumber {string} {
  set result ""
  foreach char [split $string ""] {
    if {[string first $char "0123456789"] == -1} {
      return $result
    } else {
      append result $char
    }
  }
  return $result
}

proc webify {string} {
  # Tcl8.1 only:
  #return [string map {\" &quot; & &amp; < &lt; > &gt;} $string]

  # Otherwise use this:
  regsub -all "\\&" $string "\\&amp;" string
  regsub -all "\"" $string "\\&quot;" string
  regsub -all "<" $string "&lt;" string
  regsub -all ">" $string "&gt;" string

  return $string
}

proc convstyle {string} {
  global web_color web_style
  set result ""
  set stack ""
  for {set i 0} "\$i < [string length $string]" {incr i} {
    set char [string index $string $i]
    switch -- $char {
      "\002"  - "\026" - "\037" {
        if {[string first $char $stack] != -1} {
          # NOT &&
          if {[string index $stack 0] == $char} {
            append result [lindex $web_style($char) 1]
            set stack [string range $stack 1 end]
          }
        } else {
          append result [lindex $web_style($char) 0]
          set stack $char$stack
        }
      }
      "\003" {
        if {[string first $char $stack] != -1} {
          if {[string index $stack 0] == $char} {
            append result [lindex $web_style($char) 1]
            set stack [string range $stack 1 end]
          }
        }
        set c [getnumber [string range $string [expr $i + 1] [expr $i + 2]]]
        if {$c != "" && $c >= 0 && $c <= 15} {
          incr i [string length $c]
          append result "<FONT COLOR=\"$web_color($c)\">"
          set stack $char$stack
        }
      }
      default {append result $char}
    }
  }
  foreach char [split $stack ""] {
    if {$char == "\002" || $char == "\003" ||
        $char == "\026" || $char == "\037"} {
      append result [lindex $web_style($char) 1]
    }
  }
  return $result
}

proc urlstrip {string} {
  global web_urlchars
  set result ""
  foreach char [split $string ""] {
    if {[string first $char $web_urlchars] != -1} {
      append result $char
    }
  }
  return $result
}

proc do_ques {} {
  global web_file web_update web_timerid
  global botnick timezone

  if {[info exists web_timerid]} {unset web_timerid}

  foreach chan [array names web_file] {
    if {[lsearch -exact [string tolower [channels]] [string tolower $chan]] == -1} {continue}
    set i [split $web_file($chan) ":"]
    set dir ""
    set file1 [lindex $i 0]
    set file2 [lindex $i 1]
    set j [string last "/" $file1]
    if {$j != -1} {
      set dir [string range $file1 0 $j]
      set file1 [string range $file1 [expr $j + 1] end]
    }
    unset i j
    if {$file1 != ""} {
      set fd1 [open $dir$file1~new w]
    } else {
      set fd1 [open "/dev/null" w]
    }
    if {$file2 != ""} {
      set fd2 [open $dir$file2~new w]
    } else {
      set fd2 [open "/dev/null" w]
    }

    puts $fd1 "<HTML>"
    puts $fd1 "  <HEAD>"
    puts $fd1 "    <TITLE>People on [webify $chan] right now</TITLE>"
    puts $fd1 "    <META HTTP-EQUIV=\"Refresh\" CONTENT=\"[webify [expr $web_update * 60]]\">"
    puts $fd1 "    <META NAME=\"GENERATOR\" VALUE=\"ques5.tcl\">"
    puts $fd1 "  </HEAD>"
    puts $fd1 "  <BODY>"

    puts $fd2 "<HTML>"
    puts $fd2 "  <HEAD>"
    puts $fd2 "    <TITLE>People on [webify $chan] right now</TITLE>"
    puts $fd2 "    <META HTTP-EQUIV=\"Refresh\" CONTENT=\"[webify [expr $web_update * 60]]\">"
    puts $fd2 "    <META NAME=\"GENERATOR\" VALUE=\"ques5.tcl\">"
    puts $fd2 "  </HEAD>"
    puts $fd2 "  <BODY>"
    if {![onchan $botnick $chan]} {
      puts $fd1 "    <H1>Oops!</H1>"
      puts $fd1 "    I'm not on [webify $chan] right now for some reason<BR>"
      puts $fd1 "    IRC isn't a very stable place these days..."
      puts $fd1 "    Please try again later!<BR>"

      puts $fd2 "    <H1>Oops!</H1>"
      puts $fd2 "    I'm not on [webify $chan] right now for some reason<BR>"
      puts $fd2 "    IRC isn't a very stable place these days..."
      puts $fd2 "    Please try again later!<BR>"
    } else {
      puts $fd1 "    <H1>[webify $chan]</H1>"
      puts $fd2 "    <H1>[webify $chan]</H1>"
      if {$file2 != ""} {
        puts $fd1 "    If this page looks screwy on your browser, "
        puts $fd1 "    try the <A HREF=\"$file2\">HTML 2.0 "
        puts $fd1 "    version</A>.<BR>"
      }
      puts $fd1 "    <TABLE BORDER=\"1\" CELLPADDING=\"4\">"
      puts $fd1 "      <CAPTION>People on [webify $chan] as of [webify [strftime %a,\ %d\ %b\ %Y\ %H:%M\ %Z]]</CAPTION>"
      puts $fd1 "      <TR>"
      puts $fd1 "        <TH ALIGN=\"LEFT\">Nickname</TH>"
      puts $fd1 "        <TH ALIGN=\"LEFT\">Status</TH>"
      puts $fd1 "        <TH ALIGN=\"LEFT\">User@Host</TH>"
      puts $fd1 "      </TR>"
      puts $fd2 "    <EM>People on [webify $chan] as of [webify [strftime %a,\ %d\ %b\ %Y\ %H:%M\ %Z]]</EM>"
      puts $fd2 "    <PRE>"
      puts $fd2 "      Nickname  Status           User@Host"
      foreach nick [chanlist $chan] {
        set len1 9
        set len2 16
        puts $fd1 "      <TR ALIGN=\"LEFT\" VALIGN=\"TOP\">"
        if {[isop $nick $chan]} {lappend status "op"}
        if {[getchanidle $nick $chan] > 10} {lappend status "idle"}
        set host [getchanhost $nick $chan]
        set handle [finduser $nick!$host]
        set host [webify $host]
        if {[onchansplit $nick $chan]} {
          lappend status "<STRONG>split</STRONG>"
          #incr len2 [string length "<STRONG></STRONG>"]
          incr len2 17
        }
        if {![info exists status]} {
          set status "-"
        } else {
          set status [join $status ", "]
        }
        set url [urlstrip [getuser $handle XTRA url]]
        set info [getuser $handle INFO]
        set comment [getuser $handle COMMENT]
        set email [getuser $handle XTRA email]
        if {$url == "" && [string range $comment 0 6] == "http://"} {
          set url [urlstrip $comment]
        }
        if {$url == "" && [string range $info 0 6] == "http://"} {
          set url [urlstrip $info]
        }
        if {$url == "" && $email != "" && [string match *@*.* $email]} {
          set url [urlstrip mailto:$email]
        }
        incr len1 [string length [webify $nick]]
        incr len1 -[string length $nick]
        if {[string tolower $nick] == [string tolower $botnick]} {
          set host "<EM>&lt;- it's me, the channel bot!</EM>"
          set info ""
        } elseif {[matchattr $handle b]} {
          set host "<EM>&lt;- it's another channel bot</EM>"
          set info ""
        }
        if {$url != ""} {
          incr len1 [string length "<A HREF=\"$url\"></A>"]
          puts $fd1 "        <TD><A HREF=\"$url\">[webify $nick]</A></TD>"
          puts $fd2 "      [format %-${len1}s <A\ HREF=\"$url\">[webify $nick]</A>] [format %-${len2}s $status] $host"
        } else {
          puts $fd1 "        <TD>[webify $nick]</TD>"
          puts $fd2 "      [format %-${len1}s [webify $nick]] [format %-${len2}s $status] $host"
        }
        puts $fd1 "        <TD>$status</TD>"
        puts $fd1 "        <TD>$host</TD>"
        puts $fd1 "      </TR>"
        if {$info != ""} {
          puts $fd1 "      <TR ALIGN=\"LEFT\" VALIGN=\"TOP\">"
          puts $fd1 "        <TD></TD><TD COLSPAN=\"2\"><STRONG>Info</STRONG>: [convstyle [webify $info]]</TD>"
          puts $fd1 "      </TR>"
          puts $fd2 "                <STRONG>Info:</STRONG> [convstyle [webify $info]]"
        }
        unset len1 len2 status info url host comment email
      }
      puts $fd1 "    </TABLE>"
      puts $fd2 "    </PRE>"
    }
    puts $fd1 "    <HR>"
    puts $fd1 "    This page is automatically refreshed every [webify $web_update] minute(s).<BR>"
    puts $fd1 "    <ADDRESS>Created by quesedilla v5 via <A HREF=\"http://www.eggheads.org/\">eggdrop</A>.</ADDRESS>"
    puts $fd1 "  </BODY>"
    puts $fd1 "</HTML>"
    puts $fd1 ""
    puts $fd2 "    <HR>"
    puts $fd2 "    This page is automatically refreshed every [webify $web_update] minute(s).<BR>"
    puts $fd2 "    <ADDRESS>Created by quesedilla v5 via <A HREF=\"http://www.eggheads.org/\">eggdrop</A>.</ADDRESS>"
    puts $fd2 "  </BODY>"
    puts $fd2 "</HTML>"
    puts $fd2 ""
    close $fd1
    close $fd2
    if {$file1 != ""} {exec /bin/mv $dir$file1~new $dir$file1}
    if {$file2 != ""} {exec /bin/mv $dir$file2~new $dir$file2}
    unset nick file1 file2 dir fd1 fd2
  }

  set web_timerid [timer $web_update do_ques]
}

#if {[info exists web_timerid]} {
#  killtimer $web_timerid
#  unset web_timerid
#}
if {![info exists web_timerid] && $web_update > 0} {
  set web_timerid [timer $web_update do_ques]
}
#do_ques

foreach chan [array names web_file] {
  if {[string first ":" $web_file($chan)] != -1} {
    lappend channels "$chan"
  } else {
    lappend channels "$chan (no lynx)"
  }
}

if {![info exists channels]} {
  putlog "Quesedilla v5 final loaded (no channels)"
} else {
  putlog "Quesedilla v5 final loaded: [join $channels ,\ ]"
  unset channels
}

if {![info exists timezone]} {
  set timezone [clock format 0 -format %Z]
}
