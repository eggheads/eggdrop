#
# This script just quickly maps old tcl functions to the new ones,
# use this is you are to lazy to get of your butt and update your scripts :D
#
# cleanup by Tothwolf 25May1999

proc gethosts {hand} {
  return [getuser $hand HOSTS]
}

proc addhost {hand host} {
  return [setuser $hand HOSTS $host]
}

proc chpass {hand pass} {
  return [setuser $hand PASS $pass]
}

# setxtra is no longer relevant 

proc getxtra {hand} {
  return [getuser $hand XTRA]
}

proc setinfo {hand info} {
  return [setuser $hand INFO $info]
}

proc getinfo {hand} {
  return [getuser $hand INFO]
}

proc getaddr {hand} {
  return [getuser $hand BOTADDR]
}

proc setaddr {hand addr} {
  return [setuser $hand BOTADDR $addr]
}

proc getdccdir {hand} {
  return [getuser $hand DCCDIR]
}

proc setdccdir {hand dccdir} {
  return [setuser $hand DCCDIR $dccdir]
}

proc getcomment {hand} {
  return [getuser $hand COMMENT]
}

proc setcomment {hand comment} {
  return [setuser $hand COMMENT $comment]
}

proc getemail {hand} {
  return [getuser $hand XTRA email]
}

proc setemail {hand email} {
  return [setuser $hand XTRA EMAIL $email]
}

proc getchanlaston {hand} {
  return [lindex [getuser $hand LASTON] 1]
}

proc time {} {
  return [strftime "%H:%M"]
}

proc date {} {
  return [strftime "%d %b %Y"]
}

# as you can see it takes a lot of effort to simulate all the old commands
# and adapting your scripts will take such an effort you better include
# this file forever and a day :D
