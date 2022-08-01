import requests
from bs4 import BeautifulSoup
from eggdroppy import binds, FlagMatcher, UserFlags as perm
from pprint import pprint
import eggdrop

def pubGetTitle(nick, uhost, hand, chan, text):
  print(text)
  reqs = requests.get(text)
  soup = BeautifulSoup(reqs.text, 'html.parser')
  eggdrop.putmsg(chan, "The title of the webpage is: "+soup.find_all('title')[0].get_text())

def pubmGetTitle(nick, uhost, hand, chan, text):
  print(text)
  reqs = requests.get(text.split()[1])
  soup = BeautifulSoup(reqs.text, 'html.parser')
  eggdrop.putmsg(chan, "The title of the webpage is: "+soup.find_all('title')[0].get_text())

def joinGreetUser(nick, uhost, hand, chan):
  print("JOIN BIND TRIGGERED SUCCESSFULLY")
  eggdrop.putmsg(chan, f"Hello {nick}, welcome to {chan}")

def mypub(nick, user, hand, chan, text):
  eggdrop.putmsg(chan, "!!! "+nick+"+ on "+chan+" said "+text)
  return

def joinGreetOp(nick, uhost, hand, chan):
  eggdrop.putmsg(chan, f"Hello {nick}, welcome to {chan}, you are an operator")

def mypub2(nick, user, hand, chan, text):
  print("!!! "+nick+"+ on "+chan+" said "+text+" and is a global +o")

#binds.add("pubm", "!moo", binds.FlagMatcher(), "*", mypub)
#binds.add("pubm", "!moo*", binds.FlagMatcher(globalflags=binds.UserFlags.o), "*", mypub2)
#binds.add("pub", "!title", binds.FlagMatcher(), "*", pubGetTitle)
#binds.add("pubm", "what*", binds.FlagMatcher(), "*", pubmGetTitle)
binds.join.add(joinGreetUser, FlagMatcher(), "*")
binds.join.add(joinGreetOp, FlagMatcher(globalflags=perm.op | perm.master | perm.owner, chanflags=perm.o | perm.m | perm.n), "*")
binds.pubm.add(mypub, FlagMatcher(), "*")
pprint(binds.all())
