#import py
import re
from enum import IntFlag
import eggdrop

class UserFlags(IntFlag):
  autoop = eggdrop.USER_AUTOOP
  bot = eggdrop.USER_BOT
  common = eggdrop.USER_COMMON
  deop = eggdrop.USER_DEOP
  exempt = eggdrop.USER_EXEMPT
  friend = eggdrop.USER_FRIEND
  autovoice = eggdrop.USER_GVOICE
  highlight = eggdrop.USER_HIGHLITE
  janitor = eggdrop.USER_JANITOR
  autokick = eggdrop.USER_KICK
  halfop = eggdrop.USER_HALFOP
  master = eggdrop.USER_MASTER
  owner = eggdrop.USER_OWNER
  op = eggdrop.USER_OP
  partyline = eggdrop.USER_PARTY
  devoice = eggdrop.USER_QUIET
  dehalfop = eggdrop.USER_DEHALFOP
  botnetmaster = eggdrop.USER_BOTMAST
  unshared = eggdrop.USER_UNSHARED
  voice = eggdrop.USER_VOICE
  wasoptest = eggdrop.USER_WASOPTEST
  xfer = eggdrop.USER_XFER
  autohalfop = eggdrop.USER_AUTOHALFOP
  washalfoptest = eggdrop.USER_WASHALFOPTEST

class FlagMatcher(Object):
  def __init__(self, globalflags=None, chanflags=None, requireall=False):
    self.globalflags = globalflags
    self.chanflags = chanflags
    self.requireall = requireall
  
  def match(self, globalflags, chanflags):
    if self.globalflags is not None:
      mg = self.globalflags & globalflags
    if self.chanflags is not None:
      mc = self.chanflags & chanflags
    

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
