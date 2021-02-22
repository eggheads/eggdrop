#import py
import re


class Eggdrop:
  def __init__(self, binds):
    self.binds = binds

class Binds:
  def __init__(self):
    self.bindlist = {"msgm": {}, "pubm": {}}

  def list(self, mask=None):
    if not mask:
      print(self.bindlist)
    else:
      for i in self.bindlist[mask]:
        print(self.bindlist[mask][i])
    return

  def add(self, bindtype, cmd, flags, mask, callback):
    self.bindlist[bindtype][cmd]={}
    self.bindlist[bindtype][cmd]["callback"] = callback
    self.bindlist[bindtype][cmd]["mask"] = mask
    self.bindlist[bindtype][cmd]["flags"] = flags
    return

my_binds = Binds()
eggdrop = Eggdrop(binds=my_binds)

import re
 
def bindmask2re(mask):
  r = ''
  for c in mask:
    if c == '?':
      r += '.'
    elif c == '*':
      r += '.*'
    elif c == '%':
      r += '\S*'
    elif c == '~':
      r += '\s+'
    else:
      r += re.escape(c)
  return re.compile('^' + r + '$')


def myfunc(nick, user, hand, chan, text):
  print("Holy shit this worked- "+nick+" on "+chan+" said "+text)
  return

def on_pubm(nick, user, hand, chan, text):
  print("pubm bind triggered with "+nick+" "+user+" "+hand+" "+chan+" "+text)
  for i in eggdrop.binds.bindlist["pubm"]:
    print("mask is "+eggdrop.binds.bindlist["pubm"][i]["mask"])
    eggdrop.binds.bindlist["pubm"][i]["callback"](nick, user, hand, chan, text)
  return

def on_msgm(nick, user, hand, text):
  print("msgm bind triggered with "+" ".join([nick, user, hand, text]))
  for i in eggdrop.binds.bindlist["pubm"]:
    print("mask is "+eggdrop.binds.bindlist["msgm"][i]["mask"])
    eggdrop.binds.bindlist["msgm"][i]["callback"](nick, user, hand, text)
  return

def on_event(bindtype, *args):
  for arg in args:
    print(arg)
  if bindtype == "pubm":
    on_pubm(*args)
  elif bindtype == "msgm":
    on_msgm(*args)
#  py.putserv("PRIVMSG :"+chan+" you are "+nick+" and you said "+text)
  return 0

eggdrop.binds.add("pubm", "!hi", "o|o", "*!*@*", myfunc)
