# GOAL: Demonstrate how to convert a list provided by a Tcl command into a Python list

# Load bind from eggdrop, not eggdrop.tcl. Loading it from eggdrop.tcl would cause
# the bind to call a Tcl proc, not the python method.
from eggdrop import bind, parse_tcl_list

# Load any Tcl commands you want to use from the eggdrop.tcl module.
from eggdrop.tcl import putmsg, putlog, chanlist

# And now, a totally normal python module
import random

# This is a proc that calls the putmsg Tcl command. Note that, slightly different than Tcl,
# each argument is separated by a comma instead of just a space
#
# This function is trivial and silly, but shows how Python can properly accept and type a Tcl list
def pickAFriend(nick, user, hand, chan, text, **kwargs):
  users = chanlist(chan)
  # Here we use a new python function called 'parse_tcl_list' to convert the Tcl list provided by chanlist
  # into a python list. Without this, you just have a long space-separated string
  userlist = parse_tcl_list(users)
  putlog(f"This is a python list of the users: {userlist}")
  bestFriend = random.choice(userlist)
  putmsg(chan, f"The first user I found was {userlist[0]}")
  putmsg(chan, f"But {bestFriend} is my new best friend!")

# Call binds at the end of the script, not the top- the methods must be defined!
bind("pub", "*", "!friend", pickAFriend)

print('Loaded bestfriend.py')
