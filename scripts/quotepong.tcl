# quotepong.tcl by [sL] (Feb 14, 08)
# Based on quotepass.tcl by simple, guppy, [sL]
#
# Ascii Letter definitions provided by Freeder
#
# Description:
#
# Some EFnet servers require the user to type /quote pong :<cookie>
# when ident is broken or disabled.  This will send pong :<cookie> to
# the server when connecting.
#

set a2t_alphabet(a) "\n\     /\\    \n\
\   /  \\   \n\
\  / /\\ \\  \n\
\ / ____ \\ \n\
/_/    \\_\\\n"

set a2t_alphabet(b) "\  ____  \n\
|  _ \\ \n\
| |_) |\n\
|  _ < \n\
| |_) |\n\
|____/"

set a2t_alphabet(c) "\   _____ \n\
\ / ____|\n\
| |     \n\
| |     \n\
| |____ \n\
\ \\_____|"

set a2t_alphabet(d) "\  _____  \n\
|  __ \\ \n\
| |  | |\n\
| |  | |\n\
| |__| |\n\
|_____/ "

set a2t_alphabet(e) "\  ______ \n\
|  ____|\n\
| |__   \n\
|  __|  \n\
| |____ \n\
|______|\n"

set a2t_alphabet(f) "\  ______ \n\
|  ____|\n\
| |__   \n\
|  __|  \n\
| |     \n\
|_|     "


set a2t_alphabet(g) "\   _____\n\
\ / ____|\n\
| |  __ \n\
| | |_ |\n\
| |__| |\n\
\ \\_____|"

set a2t_alphabet(h) "\  _    _ \n\
| |  | |\n\
| |__| |\n\
|  __  |\n\
| |  | |\n\
|_|  |_|"

set a2t_alphabet(i) "\  _____ \n\
|_   _|\n\
\  | |  \n\
\  | |  \n\
\ _| |_ \n\
|_____|"

set a2t_alphabet(j) "\       _ \n\
\     | |\n\
\     | |\n\
\ _   | |\n\
| |__| |\n\
\ \\____/ "

set a2t_alphabet(k) "\  _  __\n\
| |/ /\n\
| ' / \n\
|  <  \n\
| . \\ \n\
|_|\\_\\"

set a2t_alphabet(l) "\  _      \n\
| |     \n\
| |     \n\
| |     \n\
| |____ \n\
|______|"

set a2t_alphabet(m) "\  __  __ \n\
|  \\/  |\n\
| \\  / |\n\
| |\\/| |\n\
| |  | |\n\
|_|  |_|"

set a2t_alphabet(n) "\  _   _ \n\
| \\ | |\n\
|  \\| |\n\
| . ` |\n\
| |\\  |\n\
|_| \\_|"


set a2t_alphabet(o) "\   ____  \n\
\ / __ \\ \n\
| |  | |\n\
| |  | |\n\
| |__| |\n\
\ \\____/ "

set a2t_alphabet(p) "\  _____  \n\
|  __ \\ \n\
| |__) |\n\
|  ___/ \n\
| |     \n\
|_|     "

set a2t_alphabet(q) "\   ____  \n\
\ / __ \\ \n\
| |  | |\n\
| |  | |\n\
| |__| |\n\
\ \\___\\_\\"

set a2t_alphabet(r) "\  _____  \n\
|  __ \\ \n\
| |__) |\n\
|  _  / \n\
| | \\ \\ \n\
|_|  \\_\\"

set a2t_alphabet(s) "\   _____ \n\
\ / ____|\n\
| (___  \n\
\ \\___ \\ \n\
\ ____) |\n\
|_____/ "

set a2t_alphabet(t) "\  _______ \n\
|__   __|\n\
\   | |   \n\
\   | |   \n\
\   | |   \n\
\   |_|   "


set a2t_alphabet(u) "\  _    _ \n\
| |  | |\n\
| |  | |\n\
| |  | |\n\
| |__| |\n\
\ \\____/ "

      
set a2t_alphabet(v) " __      __\n\
\\ \\    / /\n\
\ \\ \\  / / \n\
\  \\ \\/ /  \n\
\   \\  /   \n\
\    \\/    "

set a2t_alphabet(w) " __          __\n\
\\ \\        / /\n\
\ \\ \\  /\\  / / \n\
\  \\ \\/  \\/ /  \n\
\   \\  /\\  /   \n\
\    \\/  \\/    "


set a2t_alphabet(x) " __   __\n\
\\ \\ / /\n\
\ \\ V / \n\
\  > <  \n\
\ / . \\ \n\
/_/ \\_\\"

set a2t_alphabet(y) " __     __\n\
\\ \\   / /\n\
\ \\ \\_/ / \n\
\  \\   /  \n\
\   | |   \n\
\   |_|   "


set a2t_alphabet(z) "\  ______\n\
|___  /\n\
\   / / \n\
\  / /  \n\
\ / /__ \n\
/_____|"

proc a2t_ascii2text {ascii {count 6}} {
  global a2t_alphabet
#  foreach line [split $ascii \n] { putlog $line }
  set a2t_result ""
  for {set i 0} {$i < $count} {incr i} {
    foreach let [split abcdefghijklmnopqrstuvwxyz ""] {
      set match 1
      set tascii $ascii
      foreach alph_line [split $a2t_alphabet($let) \n] {
        set alph_line [string range $alph_line 1 end]
        set asc_line [lindex [split $tascii \n] 0]
        set tascii [join [lrange [split $tascii \n] 1 end] \n]
        # need to fix our match pattern
        regsub -all {\\} $alph_line {\\\\} alph_line
        if {![string match "[string trim $alph_line]*" [string trim $asc_line]]} {
          set match 0
          break
        }
      }
      if {$match} { 
        append a2t_result $let
        # remove the ascii letter
        set new_ascii [list]
        foreach alph_line [split $a2t_alphabet($let) \n] {
          set alph_line [string range $alph_line 1 end]
          set asc_line [lindex [split $ascii \n] 0]
          set ascii [join [lrange [split $ascii \n] 1 end] \n]
          # need to fix our regspec
          regsub -all {\\} $alph_line {\\\\} alph_line
          regsub -all {\|} $alph_line "\\|" alph_line
          regsub -all {\)} $alph_line "\\)" alph_line
          regsub -all {\(} $alph_line "\\(" alph_line

          regsub -- $alph_line "$asc_line" "" asc_line
          lappend new_ascii $asc_line
        }
        set ascii [join $new_ascii \n]
      }
      if {$match} { break }
    }
  }
  return [string toupper $a2t_result]
}

set quotepong_match "/QUOTE PONG :cookie"

bind evnt - init-server quotepong_unbind
bind evnt - disconnect-server quotepong_unbind
bind evnt - connect-server quotepong_bind

proc quotepong_servermsg {from cmd text} {
  global quotepong_match quotepong_count quotepong_ascii
  if {![info exists quotepong_count] && [string match "*$quotepong_match*" $text]} {
    set quotepong_count 0
    set quotepong_ascii [list]
    return 0
  }
  if {[info exists quotepong_count] && ($cmd == "998")} {
    if {$quotepong_count == 0} {
      putlog "Received ASCII Cookie from server:"
    }
    incr quotepong_count
    lappend quotepong_ascii [lindex [split $text :] 1]
    putlog "[lindex [split $text :] 1]"
    if {$quotepong_count == 6} {
      # time to send back to server
      set cookie [a2t_ascii2text [join $quotepong_ascii \n]]
      putlog "Sending Cookie to server: $cookie"
      putserv "PONG :$cookie"
      catch {unset quotepong_count}
    }
  }
  return 0
}

proc quotepong_unbind {type} {
  # Try to unbind our raw NOTICE bind once we are connected since it will
  # never be needed again
  catch {
    unbind raw - NOTICE quotepong_servermsg
    unbind raw - 998 quotepong_servermsg
  }
}

proc quotepong_bind {type} {
  bind raw - NOTICE quotepong_servermsg
  bind raw - 998 quotepong_servermsg
}

