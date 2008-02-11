#
# quotepass.tcl
#   written by simple, [sL], and guppy
#
# Some servers on the Undernet will make you send 'PASS <numbers>' before you
# can connect if you did not return an identd response. This script will
# handle sending that for you.
#
# updates
# -------
#  10Feb08: initial version
#
# $Id: quotepass.tcl,v 1.3 2008/02/11 02:28:41 guppy Exp $

set quotepass_resyntax "must type /QUOTE PASS (\[^\" \]*)"

bind evnt - init-server quotepass_unbind
bind evnt - disconnect-server quotepass_unbind
bind evnt - connect-server quotepass_bind

proc quotepass_notice {from cmd text} {
  global quotepass_resyntax
  if {[regexp -nocase $quotepass_resyntax $text - pass]} {
    putlog "Got a QUOTE PASS request from the server, sending \"PASS $pass\"" 
    putserv "PASS $pass"
  }
  return 0
}

proc quotepass_unbind {type} {
  # Try to unbind our raw NOTICE bind once we are connected since it will
  # never be needed again 
  catch {
    unbind raw - NOTICE quotepass_notice
  }
}

proc quotepass_bind {type} {
  bind raw - NOTICE quotepass_notice
} 

