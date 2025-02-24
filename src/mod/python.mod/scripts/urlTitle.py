# GOAL: Demonstrate how to use a third party python module

# Load bind from eggdrop, not eggdrop.tcl. Loading it from eggdrop.tcl would cause
# the bind to call a Tcl proc, not the python function.
from eggdrop import bind

# Load any Tcl commands you want to use from the eggdrop.tcl module.
from eggdrop.tcl import putmsg, putlog

# Load a python module as usual.
from bs4 import BeautifulSoup
import requests

# Just a totally normal Python function, nothing special here! Note the Tcl command 
# 'putlog' is called
def pubGetTitle(nick, host, handle, channel, text, **kwargs):
  try:
    reqs = requests.get(text)
    soup = BeautifulSoup(reqs.text, 'html.parser')
    putlog("Found title for "+text)
    putmsg(channel, "The title of the webpage is: "+soup.find_all('title')[0].get_text())
  except Exception as e:
    putmsg(channel, "Error: " + str(e))

# Call binds at the end of the script, not the top- the functions must be defined!
bind("pub", "*", "!title", pubGetTitle)

print('Loaded urlTitle.py')
