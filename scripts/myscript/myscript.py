import requests
from bs4 import BeautifulSoup
from eggdroppy import binds, FlagMatcher, UserFlags as perm, putmsg
from pprint import pprint

def pubGetTitle(nick, host, handle, channel, text, **kwargs):
  print(text)
  reqs = requests.get(text)
  soup = BeautifulSoup(reqs.text, 'html.parser')
  putmsg(chan, "The title of the webpage is: "+soup.find_all('title')[0].get_text())

def pubmGetTitle(nick, host, handle, channel, text, **kwargs):
  print(text)
  reqs = requests.get(text.split()[1])
  soup = BeautifulSoup(reqs.text, 'html.parser')
  putmsg(chan, "The title of the webpage is: "+soup.find_all('title')[0].get_text())

def joinGreetUser(nick, host, handle, channel, **kwargs):
  putmsg(channel, f"Hello {nick}, welcome to {channel}")

# Called from pub or msg, responds public or privately
def mypub(nick, reply, **kwargs):
  reply(f"{nick}, you said !moo")
  #putmsg(chan, "!!! "+nick+"+ on "+chan+" said "+text)
  return

def joinGreetOp(nick, channel, reply, **kwargs):
  reply(f"Hello {nick}, welcome to {channel}, you are an operator")

def mypub2(nick, user, hand, chan, text, **kwargs):
  print("!!! "+nick+"+ on "+chan+" said "+text+" and is a global +o")

binds.pub.add(callback=mypub, mask="!moo")
binds.msg.add(callback=mypub, mask="!moo")
#binds.add("pubm", "!moo*", binds.FlagMatcher(globalflags=binds.UserFlags.o), "*", mypub2)
#binds.add("pub", "!title", binds.FlagMatcher(), "*", pubGetTitle)
#binds.add("pubm", "what*", binds.FlagMatcher(), "*", pubmGetTitle)
binds.join.add(callback=joinGreetUser, mask="*")
binds.join.add(callback=joinGreetOp, flags="+mno|+mno", mask="*")
pprint(binds.all())
