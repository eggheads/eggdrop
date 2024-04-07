# GOAL: Demonstrate how to convert a list provided by a Tcl command into a Python list

# Load all from eggdrop. Tcl commands must be prefixed with tcl.
from eggdrop import *

# And now, a totally normal python module
import random

# This is a proc that calls the putmsg Tcl command. Note that, slightly different than Tcl,
# each argument is separated by a comma instead of just a space
#
# This function is trivial and silly, but shows how Python can properly accept and type a Tcl list
def pickAFriend(nick, user, hand, chan, text, **kwargs):
  users = tcl.chanlist(chan)
  # Here we use a new python function called 'parse_tcl_list' to convert the Tcl list provided by chanlist
  # into a python list. Without this, you just have a long space-separated string
  userlist = parse_tcl_list(users)
  tcl.putlog(f"This is a python list of the users: {userlist}")
  bestFriend = random.choice(userlist)
  tcl.putmsg(chan, f"The first user I found was {userlist[0]}")
  tcl.putmsg(chan, f"But {bestFriend} is my new best friend!")

# Call binds at the end of the script, not the top- the methods must be defined!
bind("pub", "*", "!friend", pickAFriend)

print('Loaded bestfriend.py')
