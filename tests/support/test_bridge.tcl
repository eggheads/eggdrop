# Eggdrop integration-test bridge.
#
# Sourced from a rendered eggdrop.conf when EGGDROP_TEST is set in the
# environment. Binds a TCP listener on 127.0.0.1:0, writes the OS-assigned
# port to $env(EGGDROP_TEST_PORT_FILE), and accepts line-delimited Tcl
# commands. Each command is evaluated in the global interpreter; the result
# is returned as a single line tagged "OK" or "ERR".
#
# Wire format (one line per frame, \n-terminated):
#   request:   <escaped command>\n
#   response:  OK <escaped result>\n   or   ERR <escaped result>\n
# where \, \n, \r in the payload are backslash-escaped so each frame is
# always exactly one line.
#
# Eggdrop's Tcl_ServiceAll() runs every main-loop tick, so socket -server
# and fileevent fire normally. This script is *only* loaded under the
# EGGDROP_TEST gate and must never ship enabled in production.

namespace eval ::eggtest {
    proc escape {s} {
        return [string map [list "\\" "\\\\" "\n" "\\n" "\r" "\\r"] $s]
    }

    proc unescape {s} {
        return [string map [list "\\\\" "\\" "\\n" "\n" "\\r" "\r"] $s]
    }

    proc on_data {sock} {
        if {[catch {gets $sock line} n] || $n < 0} {
            if {[eof $sock]} {
                close_sock $sock
            }
            return
        }
        set cmd [unescape $line]
        if {[catch {uplevel #0 $cmd} result]} {
            puts $sock "ERR [escape $result]"
        } else {
            puts $sock "OK [escape $result]"
        }
        flush $sock
    }

    proc close_sock {sock} {
        catch {close $sock}
    }

    proc on_accept {sock host port} {
        fconfigure $sock -blocking 0 -translation lf -buffering line
        fileevent $sock readable [list ::eggtest::on_data $sock]
    }

    proc start {} {
        if {![info exists ::env(EGGDROP_TEST_PORT_FILE)]} {
            putlog "test_bridge: EGGDROP_TEST_PORT_FILE not set, refusing to start"
            return
        }
        set srv [socket -server ::eggtest::on_accept -myaddr 127.0.0.1 0]
        set port [lindex [fconfigure $srv -sockname] 2]
        set portfile $::env(EGGDROP_TEST_PORT_FILE)
        set tmp "$portfile.tmp"
        set f [open $tmp w]
        puts -nonewline $f $port
        close $f
        file rename -force $tmp $portfile
        putlog "test_bridge listening on 127.0.0.1:$port (wrote $portfile)"
    }
}

::eggtest::start
