package require json
package require json::write
package require http
package require tls


bind dcc * egg parse_egg
bind dcc * scripts parse_egg

set eggdir "autoscripts"

set jsondict [dict create]

#Read all JSON script files
proc readjsonfile {} {
  global jsondict
  global eggdir
  set jsonstring "\["
### How to get filepath properly
  foreach dirname [glob -nocomplain -type d -directory $eggdir *] {
putlog "doing $dirname"
    if {![file exists $dirname/manifest.json]} {
      file delete -force $dirname
      putlog "$dirname missing manifest.json, deleting"
    } else {
      set fs [open $dirname/manifest.json r]
      if {[string length $jsonstring] > 1} {
        append jsonstring ","
      }
      set filestring [read $fs]
      append jsonstring $filestring
      close $fs
    }
  }
  append jsonstring "\]"
  set jsondict [json::json2dict $jsonstring]
}

# Write a script's JSON content to file
proc write_json {script jstring} {
  global eggdir
  set fs [open $eggdir/${script}/manifest.json w]
  puts $fs $jstring
  close $fs
}

# Send an HTTP request. Type 0 for text, 1 for binary
proc send_http {url type} {
  global eggdir
  http::register https 443 [list ::tls::socket -autoservername true]
  set req [http::config -useragent "eggdrop"]
  if {$type} {
    set fs [open $eggdir/[lindex [split $url /] end] w]
    catch {set req [http::geturl $url -binary 1 -channel $fs]} error
    close $fs
  } else {
    catch {set req [http::geturl $url -timeout 10000]} error
  }
  set status [http::status $req]
  if {$status != "ok"} {
    putlog "HTTP request error: $error"
    return
  }
  set data [http::data $req]
  ::http::cleanup $req
  return $data
}

# Read all manifest files
proc loadscripts {} {
  global jsondict
  global eggdir
  foreach scriptentry $jsondict {
    if [dict get $scriptentry config loaded] {
      if {[dict exists $scriptentry config vars]} {
        foreach configvar [dict keys [dict get $scriptentry config vars] *] {
          uplevel #0 set $configvar [dict get $scriptentry config vars $configvar value]
        }
      }
      if {[catch {uplevel #0 source $eggdir/[dict get $scriptentry name]/[dict get $scriptentry name].tcl} err]} {
        putlog "Error loading [dict get $scriptentry]: $err"
        return
      }
    }
  }
}

# Initial function called by .egg command, sends to proper proc based on args
proc parse_egg {hand idx text} {
	set args [split $text]
	set args [lassign $args subcmd arg1 arg2]
	if {$subcmd in {remote}} {
		egg_$subcmd $idx
	} elseif {$subcmd in {config fetch clean}} {
		if {$arg1 eq ""} {
		  putdcc $idx "Missing parameter, must be $::lastbind $subcmd scriptName"
		} else {
		  egg_$subcmd $idx $arg1
		}
	} elseif {$subcmd in {load unload}} {
		if {$arg1 eq ""} {
		  putdcc $idx "Missing parameter, must be $::lastbind $subcmd scriptName"
		} else {
		  egg_load $idx $arg1 [expr {$subcmd eq "load"}]
		}
	} elseif {$subcmd in {set}} {
		if {![llength $args]} {
		  putdcc $idx "Missing parameter, must be $::lastbind $subcmd scriptName settingName newSettingValue"
		} else {
		  egg_$subcmd $idx $arg1 $arg2 [join $args]
		}
    } elseif {$subcmd in {loaded unloaded all}} {
       egg_$subcmd
	} else {
		putdcc $idx "Missing or unknown subcommand, must be $::lastbind set, config, load, unload, remote, fetch, clean"
	}
}

# List scripts that are locally present in .egg
proc egg_list {idx} {
  global jsondict
  readjsonfile
  putdcc $idx "\nThe following scripts are available for configuration:"
  putdcc $idx "------------------------------------------------------"
  foreach script $jsondict {
    set loaded [expr {[dict get $script config loaded] == 1 ? "\[X\]" : "\[ \]"}]
    putdcc $idx "* $loaded [dict get $script name] (v[dict get $script version_major].[dict get $script version_minor]) - [dict get $script description]"
    if {[dict exists $script config requires] && [string length [dict get $script config requires]]} {
      foreach pkg [dict get $script config requires] {
        if {[catch {[package require [dict get $script $pkg]]}]} {
          putdcc $idx "      ( ^ Must install Tcl $pkg package before loading)"
        }
      }
    }
  }
}

# Load or unload a script and update JSON field
# loadme is 0 to unload a script, 1 to load a script
proc egg_load {idx script loadme} {
  global jsondict
  global eggdir
  set found 0
  readjsonfile
  foreach scriptentry $jsondict {
    if [string match $script [dict get $scriptentry name]] {
      set found 1
      if {$loadme} {
        if {[dict exists $scriptentry config vars]} {
          foreach configvar [dict keys [dict get $scriptentry config vars] *] {
            uplevel #0 set $configvar [dict get $scriptentry config vars $configvar value]
          }
        }
        if {[catch {uplevel #0 source $eggdir/${script}/${script}.tcl} err]} {
          putdcc $idx "Error loading ${script}: $err"
          return
        }
        dict set scriptentry config loaded 1
        putdcc $idx "* Loaded $script."
      } else {
        dict set scriptentry config loaded 0
        putdcc $idx "* Removed $script from being loaded. Restart Eggdrop to complete."
        set found 1
      }
      write_json $script [compile_json {dict config {dict vars {dict * dict}}} $scriptentry]
      readjsonfile
      break
    }
  }
  if {!$found} {
    putdcc $idx "* $script not found."
  }
  return $found
}

# List variables available for a script
proc egg_config {idx script} {
  global jsondict
  foreach scriptentry $jsondict {
    if {[string match $script [dict get $scriptentry name]]} {
      if {[dict exists $scriptentry long_description]} {
        putdcc $idx "\n* [dict get $scriptentry long_description]\n\n"
      }
      if {![dict exists $scriptentry config vars]} {
        putdcc $idx "* No variables are available to configure for $script"
      } else {
        putdcc $idx "* The following config options are available for $script:"
        putdcc $idx "---------------------------------------------------------"
        foreach configvar [dict keys [dict get $scriptentry config vars] *] {
          putdcc $idx "* $configvar - [dict get $scriptentry config vars $configvar description] (current value: [dict get $scriptentry config vars $configvar value])"
        }
        if {[dict exists $scriptentry config udefflag]} {
          putdcc $idx ""
          putdcc $idx "* Enable for a channel via .chanset <channel> +[dict get $scriptentry config udefflag]"
        }
      }
    }
  }
}

# Set a variable for a script
proc egg_set {idx script setting value} {
  global jsondict
  foreach scriptentry $jsondict {
    if {[string match $script [dict get $scriptentry name]]} {
      if [dict exists $scriptentry config vars $setting] {
        dict set scriptentry config vars $setting value $value
        write_json $script [compile_json {dict config {dict vars {dict * dict}}} $scriptentry]
        putdcc $idx "* ${script}: Variable \"$setting\" set to \"${value}\""
        readjsonfile        
      }
    }
  }
}

# Pull down remote Tcl file listing
# For future eggheads- 100 is the maximum WP supports without doing pagination
proc egg_remote {idx} {
  set jsondata [send_http "https://www.eggheads.org/wp-json/wp/v2/media?mime_type=application\/x-gzip&orderby=slug&per_page=100&order=asc" 0]
  set datadict [json::json2dict $jsondata]
  putdcc $idx "* Retrieving script list, please wait..."
  putdcc $idx "Scripts available for download:"
  putdcc $idx "-------------------------------"
  foreach scriptentry $datadict {
    putdcc $idx "* [dict get $scriptentry slug] ([lindex [regexp -inline {\n.*<p>([^<>]*)</p>} [dict get $scriptentry description rendered]] 1])"
  }
  putdcc $idx "\n"
  putdcc $idx "* Type '.egg fetch <scriptname>' to download a script"
}

# Helper command for scripts- return a Tcl list of scripts that are loaded
proc egg_loaded {} {
  global jsondict
  list scriptlist
  foreach scriptentry $jsondict {
    if {[dict get $scriptentry config loaded]} {
      lappend scriptlist [dict get $scriptentry name]
    }
  }
  return $scriptlist
}

# Helper command for scripts- return a Tcl list of scripts that are loaded
proc egg_unloaded {} {
  global jsondict
  list scriptlist
  foreach scriptentry $jsondict {
    if {![dict get $scriptentry config loaded]} {
      lappend scriptlist [dict get $scriptentry name]
    }
  }
  return $scriptlist
}

# Helper command for scripts- return a Tcl list of scripts that are loaded
proc egg_all {} {
  global jsondict
  list scriptlist
  foreach scriptentry $jsondict {
    lappend scriptlist [dict get $scriptentry name]
  }
  return $scriptlist
}


# Download a script from the eggheads repository
proc egg_fetch {idx script} {
  global eggdir
  try {
    set results [exec which tar]
    set status 0
  } trap CHILDSTATUS {results options} {
    if {lindex [dict get $options -errorcode] 2} {
      putdcc $idx "* ERROR: This feature is not available (tar not detected on this system, cannot extract a downloaded file)."
      return
    }
  }
  if {[file isdirectory $eggdir/$script]} {
    putdcc $idx "* Script directory already exists. Not downloading again!"
    return
  }
### CHECK IF IT IS ON SITE FIRST
  putdcc $idx "* Downloading, please wait..."
  set jsondata [send_http "https://www.eggheads.org/wp-json/wp/v2/media?mime_type=application\/x-gzip" 0]
  set datadict [json::json2dict $jsondata]
  foreach scriptentry $datadict {
    if {[string match $script [dict get $scriptentry slug]]} {
      send_http [dict get $scriptentry source_url] 1
      file mkdir $eggdir/[dict get $scriptentry slug]
      exec tar -zxf $eggdir/[dict get $scriptentry slug].tgz -C $eggdir/[dict get $scriptentry slug]
      file delete $eggdir/[dict get $scriptentry slug].tgz
      putdcc $idx "* [dict get $scriptentry slug] downloaded."
      putdcc $idx "* Use '.egg load [dict get $scriptentry slug]' to load and '.egg config [dict get $scriptentry slug]' to configure."
      readjsonfile
    }
  }
}

proc egg_clean {idx script} {
  global eggdir
  if {![egg_load $idx $script 0]} {
    putdcc $idx "* Cannot remove $script"
    return
  }
  if {[file isdirectory $eggdir/$script]} {
    file delete -force $eggdir/$script
    putdcc $idx "* $script deleted"
  } else {
    putdcc $idx "* $script not found"
  }
}

# Yikes.
proc compile_json {spec data} {
	while [llength $spec] {
		set type [lindex $spec 0]
		set spec [lrange $spec 1 end]

		switch -- $type {
			dict {
				lappend spec * unknown

				set json {}
				foreach {key val} $data {
					foreach {keymatch valtype} $spec {
						if {[string match $keymatch $key]} {
							if {$key in {name displayname}} { set valtype string }
							lappend json [subst {"$key":[compile_json $valtype $val]}]
							break
						}
					}
				}
				return "{[join $json ,]}"
			}
			list {
				if {![llength $spec]} {
					set spec unknown
				} else {
					set spec [lindex $spec 0]
				}
				set json {}
				foreach {val} $data {
					lappend json [compile_json $spec $val]
				}
				return "\[[join $json ,]\]"
			}
			number {
				if {$data eq "" || $data eq "null"} {
					return null
				} else {
					return $data
				}
			}
			string {
				if {$data eq "" || $data eq "null"} {
					return null
				} else {
					set new ""
					for {set i 0} {$i < [string length $data]} {incr i} {
						set c [string index $data $i]
						set cc [scan $c %c]
						if {$cc < 0x7e && $c ne "\\" && $cc >= 0x20 && $c ne "\""} {
							append new $c
						} else {
							append new "\\u[format %04x $cc]"
						}
					}
					return "\"$new\""
				}
			}
			unknown {
				if {$data eq "" || $data eq "null"} {
					return null
				} elseif {[string is double -strict $data] && ![string equal -nocase $data infinity] && ![string equal -nocase $data nan] && ![string equal -nocase $data -infinity]} {
					return $data
				} else {
					set new ""
					for {set i 0} {$i < [string length $data]} {incr i} {
						set c [string index $data $i]
						set cc [scan $c %c]
						if {$cc < 0x7e && $c ne "\\" && $cc >= 0x20 && $c ne "\""} {
							append new $c
						} else {
							append new "\\u[format %04x $cc]"
						}
					}
					return "\"$new\""
				}
			}
		}
	}
}

if {![file exists autoscripts]} {file mkdir autoscripts}
readjsonfile
loadscripts
putlog "Loading foo.tcl"
