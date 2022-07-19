from eggdroppy import binds
from pprint import pprint

def mypub(nick, user, hand, chan, text):
  print("!!! "+nick+"+ on "+chan+" said "+text)
  return

def mypub2(nick, user, hand, chan, text):
  print("!!! "+nick+"+ on "+chan+" said "+text+" and is a global +o")
  return

binds.add("pubm", "!moo", binds.FlagMatcher(), "*", mypub)
binds.add("pubm", "!moo*", binds.FlagMatcher(globalflags=binds.UserFlags.o), "*", mypub2)
pprint(binds.list())
