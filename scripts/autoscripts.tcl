package require json
package require json::write
package require http
package require tls

set asidx 0
set eggdir "autoscripts"
set cmdtxt "\nEnter your command (done to exit):"
set jsondict [dict create]
set asmajor 1
set asminor 0

bind DCC n autoscript console

proc console {hand idx arg} {
  global echostatus
  global oldchan
  global asidx
  set oldchan [getchan $idx]
  set echostatus [echo $idx]
  set asidx $idx
  setchan $idx 469
  echo $idx 0
  bind FILT n * parse_egg
  bind evnt n prerehash {egg_done $asidx}
  putdcc $idx "   _         _                      _       _  "
  putdcc $idx "  /_\\  _   _| |_ ___  ___  ___ _ __(_)_ __ | |_ "
  putdcc $idx " //_\\\\| | | | __/ _ \\/ __|/ __| '__| | '_ \\| __|"
  putdcc $idx "/  _  \\ |_| | || (_) \\__ \\ (__| |  | | |_) | |_ "
  putdcc $idx "\\_/ \\_/\\__,_|\\__\\___/|___/\\___|_|  |_| .__/ \\__|"
  putdcc $idx "                                     |_|        "
  putdcc $idx "=======================================================\n"
  putdcc $idx "Welcome to the autoscript console. Enter your command: "
}

#Read all JSON script files
proc readjsonfile {} {
  global jsondict
  global eggdir
  set jsonlist {}
### How to get filepath properly
  foreach dirname [glob -nocomplain -type d -directory $eggdir *] {
    if {![file exists $dirname/manifest.json]} {
      file delete -force $dirname
      putlog "$dirname missing manifest.json, deleting"
    } else {
      set fs [open $dirname/manifest.json r]
      set contents [read $fs]
      set jcontents [json::json2dict $contents]
      if {[dict exists $jcontents schema]} {
        if {![string equal [dict get $jcontents schema] "1"]} {
          putlog "$dirname contains invalid manifest.json format, skipping..."
          continue
        }
      }
      lappend jsonlist $contents
      close $fs
    }
  }
  set jsondict [json::json2dict "\[[join $jsonlist {,}]\]"]
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
    catch {set req [http::geturl $url -headers [dict create "User-Agent" "Mozilla/5.0"] -timeout 10000]} error
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

proc createvarns {varname} {
  for {set i 0} {$i < [llength [split $varname ::]] - 1} {incr i} {
    namespace eval [join [lrange [split $varname ::] 0 $i] ::] {}
  }
}

# Read all manifest files
proc loadscripts {} {
  global jsondict
  global eggdir
  foreach scriptentry $jsondict {
    if [dict get $scriptentry config loaded] {
      if {[dict exists $scriptentry config vars]} {
        foreach configvar [dict keys [dict get $scriptentry config vars] *] {
          uplevel #0 [list createvarns $configvar]
          set ::$configvar [dict get $scriptentry config vars $configvar value]
        }
      }
      if {[catch {uplevel #0 [list source $eggdir/[dict get $scriptentry name]/[dict get $scriptentry name].tcl]} err]} {
        putlog "Error loading [dict get $scriptentry name]: $err"
        return
      }
    }
  }
}

# Initial function called from autoscript console, sends to proper proc based on args
proc parse_egg {idx text} {
    global echostatus
    global oldchan
    global asidx

    # Check if this is the user who triggered the console
    if {$idx != $asidx} {
      return
    }
	set args [split $text]
	set args [lassign $args subcmd arg1 arg2]

    if {$subcmd in {done}} {
      egg_done $idx "parse"
    } elseif {$subcmd in {remote list help}} {
		egg_$subcmd $idx
    } elseif {$subcmd in {update}} {
        egg_$subcmd $idx $arg1
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
    } elseif {$subcmd in {loaded unloaded all help}} {
       egg_$subcmd
	} else {
		putdcc $idx "Missing or unknown subcommand"
        egg_help $idx
	}
    return
}

# List scripts that are locally present in .egg
proc egg_list {idx} {
  global jsondict
  global cmdtxt
  readjsonfile
  putdcc $idx "\nThe following scripts are available for configuration:"
  putdcc $idx "-------------------------------------------------------"
  foreach script $jsondict {
    set loaded [expr {[dict get $script config loaded] == 1 ? "\[X\]" : "\[ \]"}]
    putdcc $idx "* $loaded [dict get $script name] (v[dict get $script version_major].[dict get $script version_minor]) - [dict get $script description]"
    if {[dict exists $script config requires] && [string length [dict get $script config requires]]} {
      foreach pkg [dict get $script config requires] {
        if {![string equal $pkg "null"]} {
          if {![lsearch -exact [package names] $pkg]} {
            putdcc $idx "      ( ^ Must install Tcl $pkg package on host before loading)"
          }
        }
      }
    }
  }
  putidx $idx "Additional scripts available for download can be viewed with the 'remote' command"
  putidx $idx "$cmdtxt"
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
            uplevel #0 [list createvarns $configvar]
            set ::$configvar [dict get $scriptentry config vars $configvar value]
          }
        }
        if {[catch {uplevel #0 [list source $eggdir/${script}/${script}.tcl]} err]} {
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
  global cmdtxt
  global jsondict
  set found 0
  foreach scriptentry $jsondict {
    if {[string match $script [dict get $scriptentry name]]} {
      set found 1
      if {[dict exists $scriptentry long_description]} {
        putdcc $idx "\n* [dict get $scriptentry long_description]\n\n"
      }
      if {![dict exists $scriptentry config vars]} {
        putdcc $idx "* No variables are available to configure for $script"
      } else {
        putdcc $idx "* The following config options are available for $script:"
        putdcc $idx "---------------------------------------------------------"
        putdcc $idx "Variables available for configuration via the set command: "
        foreach configvar [dict keys [dict get $scriptentry config vars] *] {
          set printstr "* $configvar - [dict get $scriptentry config vars $configvar description]"
          if {([string length $printstr] > 110) || ([string first "\n" $printstr] ne -1)} {
            if {[string first "\n" [string range $printstr 0 110]] ne -1} {
              set index [string first "\n" [string range $printstr 0 110]]
            } else {
              set index [string last { } $printstr 109]
            }
            putdcc $idx "[string range $printstr 0 $index]"
            set printstr [string range $printstr $index+1 end]

            while {[string length $printstr] > 0} {
              if {([string length $printstr] > 102) || ([string first "\n" $printstr] ne -1)} {
                if {[string first "\n" [string range $printstr 0 102]] ne -1} {
                  set index [string first "\n" [string range $printstr 0 102]]
                } else {
                  set printstr [string trimleft $printstr]
                  set index [string last { } $printstr 101]
                }
                putdcc $idx "      [string range $printstr 0 $index]"
                set printstr [string range $printstr $index+1 end]
              } else {
                putdcc $idx "      [string trimleft $printstr]"
                putdcc $idx "      (current value: [dict get $scriptentry config vars $configvar value])"
                putdcc $idx "\n"
                break
              }
            }
          } else {
            putdcc $idx "$printstr"
            putdcc $idx "      (current value: [dict get $scriptentry config vars $configvar value])"
            putdcc $idx "\n"
          }
        }
        # treats udef
        putdcc $idx "\nChannel settings available for configuration via .chanset: "
        if {[dict exists $scriptentry config udef]} {
          foreach udef [dict keys [dict get $scriptentry config udef]] {
            set utype [dict get $scriptentry config udef $udef type]
            set uval null
            if {[dict exists $scriptentry config udef $udef value]} {
              set uval [dict get $scriptentry config udef $udef value]
            }
            switch -nocase -- $utype {
              "flag" { putdcc $idx "* $udef ($utype) : [dict get $scriptentry config udef $udef description] .chanset <channel> +$udef" }
              "str" -
              "int" { putdcc $idx "* $udef ($utype) : [dict get $scriptentry config udef $udef description] .chanset <channel> $udef $uval" }
              default { putdcc $idx "* $udef seems to exist but is not well defined" }
            }
          }
          putdcc $idx ""
        }
      }
    }
  }
  if {!$found} {
    putdcc $idx "Script $script not found."
  }
  putidx $idx "$cmdtxt"
}

# Set a variable for a script
proc egg_set {idx script setting value} {
  global cmdtxt
  global jsondict
  set noscript 1
  set noset 1
  foreach scriptentry $jsondict {
    if {[string match $script [dict get $scriptentry name]]} {
      set noscript 0
      if [dict exists $scriptentry config vars $setting] {
        set noset 0
        dict set scriptentry config vars $setting value $value
        write_json $script [compile_json {dict config {dict vars {dict * dict}}} $scriptentry]
        putdcc $idx "* ${script}: Variable \"$setting\" set to \"${value}\""
        putdcc $idx "* Use \"load $script\" to enable this change"
        readjsonfile        
      }
    }
  }
  if $noscript {
    putdcc $idx "ERROR: Script \"${script}\" not found."
  } elseif $noset {
    putdcc $idx "* ${script}: Setting \"$setting\" not found."
  }
  putidx $idx "$cmdtxt"
}

# Pull down remote Tcl file listing
# For future eggheads- 100 is the maximum WP supports without doing pagination
proc egg_remote {idx} {
  global cmdtxt
  global eggdir
  putdcc $idx "* Retrieving script list, please wait..."
  send_http "https://raw.githubusercontent.com/eggheads/autoscripts/master/manifest" 1
  set fp [open $eggdir/manifest r]
  set jsondata [read $fp]
  close $fp
  set datadict [json::json2dict $jsondata]
  putdcc $idx "Scripts available for download:"
  putdcc $idx "-------------------------------"
  foreach scriptentry $datadict {
#    regsub -all {<[^>]+>} [dict get $scriptentry caption rendered] "" scriptcap
    putdcc $idx "* [format "%-16s %s" [dict get $scriptentry name] [dict get $scriptentry description]]"
  }
  putdcc $idx "\n"
  putdcc $idx "* Type 'fetch <scriptname>' to download a script"
  putidx $idx "$cmdtxt"
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

proc egg_update {idx tgtscript} {
  global cmdtxt
  global jsondict
  global version
  global asminor
  global asmajor
  set found 0

  readjsonfile
  set jsondata [send_http "https://raw.githubusercontent.com/eggheads/autoscripts/master/manifest" 0]
  set datadict [json::json2dict $jsondata]
  foreach localscript $jsondict {
    foreach remotescript $datadict {
      if {[string equal -nocase [dict get $remotescript name] [dict get $localscript name]]} {
        if { ([dict get $remotescript version_minor] > [dict get $localscript version_minor] &&
              [dict get $remotescript version_major] >= [dict get $localscript version_major]) ||
             ([dict get $remotescript version_major] > [dict get $localscript version_major]) } {
          ## If we're looking for a specific script, suppress other found messages
          if {[string equal -nocase $tgtscript ""]} {
              putdcc $idx "* [dict get $localscript name] has an update available."
            }
          set found 1
          if {[string equal -nocase $tgtscript [dict get $localscript name]]} {
            putdcc $idx "* Script update feature goes here- coming soon!"
          }
        }
      }
    }
  }
  if {!$found} {
    putdcc $idx "* No updates available."
  }
}

  foreach script $jsondict {
    set loaded [expr {[dict get $script config loaded] == 1 ? "\[X\]" : "\[ \]"}]
    putdcc $idx "* $loaded [dict get $script name] (v[dict get $script version_major].[dict get $script version_minor]) - [dict get $script description]"


  set asversion [send_http "https://www.eggheads.org/wp-content/uploads/simple-file-list/autoscript.txt" 0]
  lassign [split $asversion .] major minor
  if { ($minor > $asminor && $major >= $asmajor) || ($major > $asmajor)} {
    putdcc $idx "* New version of autoscript found!"
  } else {
    putdcc $idx "* autoscript is up to date."
  }
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
  global cmdtxt
  global eggdir
  try {
    set results [exec which tar]
    set status 0
  } trap CHILDSTATUS {results options} {
    if {lindex [dict get $options -errorcode] 2} {
      putdcc $idx "* ERROR: This feature is not available (tar not detected on this system, cannot extract a downloaded file)."
      putidx $idx "$cmdtxt"
      return
    }
  }
  if {[file isdirectory $eggdir/$script]} {
    putdcc $idx "* Script directory already exists. Not downloading again!"
    putdcc $idx "* Use \"update $script\" if you're trying to update the script"
    putidx $idx "$cmdtxt"
    return
  }
  putdcc $idx "* Downloading, please wait..."
  set jsondata [send_http "https://api.github.com/repos/eggheads/autoscripts/contents/packages" 0]
  set datadict [json::json2dict $jsondata]
  foreach scriptentry $datadict {
    if {[string match ${script}.tgz [dict get $scriptentry name]]} {
      send_http "[dict get $scriptentry download_url]" 1
      putdcc $idx "* [dict get $scriptentry name] downloaded."
      exec tar -zxf $eggdir/[dict get $scriptentry name] -C $eggdir
      if {[file exists $eggdir/$script]} {
        file delete $eggdir/[dict get $scriptentry name]
        putdcc $idx "* [dict get $scriptentry name] extracted."
        putdcc $idx "* Use 'config $script' to configure and then 'load $script' to load."
        putidx $idx "$cmdtxt"
        readjsonfile
      } else {
        putdcc $idx "* ERROR: [dict get $scriptentry name] not found. Cannot continue."
      }
    }
  }
}


proc egg_clean {idx script} {
  global cmdtxt
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
  putidx $idx "$cmdtxt"
}

proc egg_done {idx arg} {
  global oldchan
  global echostatus
  putdcc $idx "Returning to partyline..."
  unbind FILT n * parse_egg
  unbind EVNT n prerehash {egg_done $asidx}
  echo $idx $echostatus
  setchan $idx $oldchan
}


proc egg_help {idx} {
  global cmdtxt
  putidx $idx "* The following commands are available for use with $::lastbind:"
  putidx $idx "    remote, fetch, list, config, set, load, unload, clean, done"
  putidx $idx ""
  putidx $idx "* remote                 : List scripts available for download"
  putidx $idx "* fetch <script>         : Download the script"
  putidx $idx "* list                   : List all scripts present on the local system available for use"
  putidx $idx "* config <script>        : View configuration options for a script"
  putidx $idx "* set <script> <setting> : Set the value for a script setting"
  putidx $idx "* load <script>          : Load a script, and enable a script to be loaded when Eggdrop starts"
  putidx $idx "* unload <script>        : Prevent a script from running when Eggdrop starts"
  putidx $idx "                           (You must restart Eggdrop to stop a currently running script!)"
  putidx $idx "* clean <script>         : Permanently remove a script and any associated settings or files"
  putidx $idx "* update \[script\]        : Check for updates for autoscript, or specify a script to update"
  putidx $idx "* done                   : Return to Eggdrop partyline"
  putidx $idx "----------------------------------------------------------------------------------------------"
  putidx $idx "$cmdtxt"
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
putlog "Loading autoscripts.tcl"
