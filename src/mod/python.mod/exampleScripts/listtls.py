# GOAL: Demonstrate how to convert Tcl dicts nested in a Tcl list to a Python list of Python dicts

# Load bind from eggdrop, not eggdrop.tcl. Loading it from eggdrop.tcl would cause
# the bind to call a Tcl proc, not the python method.
from eggdrop import bind

# Load any Tcl commands you want to use from the eggdrop.tcl module.
# Note we also import the non-standard 'parse_tcl_list' 
from eggdrop.tcl import putmsg, putlog, socklist, parse_tcl_list, parse_tcl_dict

# This is a proc that calls the putmsg Tcl command. Note that, slightly different than Tcl,
# each argument is separated by a comma instead of just a space
#
# This function is trivial and silly, but shows how Python can properly accept and type a Tcl dict
def listInsecureSockets(nick, user, hand, chan, text, **kwargs):
  sockets = socklist()
  # socklist() will return us a Tcl-formatted list of dicts (aka, a string), so we have to first
  # convert this to a Python list using parse_tcl_list().
  socketlist = parse_tcl_list(sockets)
  # Now socklist contains a Python list of Tcl-formatted dicts (again, strings), so now we have
  # to format each list item into a Python dict
  for socket in socketlist:
    socketdict = parse_tcl_dict(socket)
    i = socketdict.get("idx")
    status = socketdict.get("secure")
    putmsg(chan, f"The TLS status of idx {i} is {status}")

# Call binds at the end of the script, not the top- the methods must be defined!
bind("pub", "*", "!listtls", listInsecureSockets)

print('Loaded listtls.py')
