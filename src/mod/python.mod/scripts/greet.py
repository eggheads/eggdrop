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

# For now, unfortunately if Eggdrop is rehashed, previously existing binds will be duplicated.
# This is example code to check for previously-existbing binds after the script reloaded and 
# delete them.
if 'GREET_BINDS' in globals():
  for greetbind in GREET_BINDS:
    greetbind.unbind()
  del GREET_BINDS

# Continuing from the above section of code, the GREET_BINDS list is created to track the binds
# created by this script, and then the binds are added to it.
GREET_BINDS = list()
# Call binds at the end of the script, not the top- the functions must be defined!
GREET_BINDS.append(bind("join", "*", "*", joinGreetUser))
# Again, arguments are separated with a comma. This bind requires the 'o' flag to be triggered.
GREET_BINDS.append(bind("join", "o", "*", joinGreetOp))

print('Loaded greet.py')
