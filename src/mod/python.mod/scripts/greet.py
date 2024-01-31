# GOAL: Demonstrate how to use a Python bind to call a Python function

# Load bind from eggdrop, not eggdrop.tcl. Loading it from eggdrop.tcl would cause
# the bind to call a Tcl proc, not the python function.
from eggdrop import bind

# Load any Tcl commands you want to use from the eggdrop.tcl module.
from eggdrop.tcl import putmsg

# This is a proc that calls the putmsg Tcl command. Note that, slightly different than Tcl,
# each argument is separated by a comma instead of just a space
def joinGreetUser(nick, host, handle, channel, **kwargs):
  putmsg(channel, f"Hello {nick}, welcome to {channel}")

def joinGreetOp(nick, host, handle, channel, **kwargs):
  putmsg(channel, f"{nick} is an operator on this channel!")

# Call binds at the end of the script, not the top- the functions must be defined!
bind("join", "*", "*", joinGreetUser)
# Again, arguments are separated with a comma. This bind requires the 'o' flag to be triggered.
bind("join", "o", "*", joinGreetOp)

print('Loaded greet.py')
