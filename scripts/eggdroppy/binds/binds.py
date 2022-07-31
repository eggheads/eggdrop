import re
import uuid
from enum import IntFlag
import eggdrop
from pprint import pprint

class UserFlags(IntFlag):
  autoop = a = eggdrop.USER_AUTOOP
  bot = b = eggdrop.USER_BOT
  common = c = eggdrop.USER_COMMON
  deop = d = eggdrop.USER_DEOP
  exempt = e = eggdrop.USER_EXEMPT
  friend = f = eggdrop.USER_FRIEND
  autovoice = g = eggdrop.USER_GVOICE
  highlight = h = eggdrop.USER_HIGHLITE
  janitor = j = eggdrop.USER_JANITOR
  autokick = k = eggdrop.USER_KICK
  halfop = l = eggdrop.USER_HALFOP
  master = m = eggdrop.USER_MASTER
  owner = n = eggdrop.USER_OWNER
  op = o = eggdrop.USER_OP
  partyline = p = eggdrop.USER_PARTY
  devoice = q = eggdrop.USER_QUIET
  dehalfop = r = eggdrop.USER_DEHALFOP
  botnetmaster = t = eggdrop.USER_BOTMAST
  unshared = u = eggdrop.USER_UNSHARED
  voice = v = eggdrop.USER_VOICE
  wasoptest = w = eggdrop.USER_WASOPTEST
  xfer = x = eggdrop.USER_XFER
  autohalfop = y = eggdrop.USER_AUTOHALFOP
  washalfoptest = z = eggdrop.USER_WASHALFOPTEST

class FlagRecord:
  def __init__(self, globalflags=None, chanflags=None, botflags=None):
    self.globl = UserFlags(globalflags)
    self.chan = UserFlags(chanflags)
    self.bot = UserFlags(botflags)
  
  def __str__(self):
    return 'globl: {}, chan: {}, bot: {}'.format(str(self.globl), str(self.chan), str(self.bot))

  def __repr__(self):
    return repr({'global': repr(self.globl), 'chan': repr(self.chan), 'bot': repr(self.bot)})

class FlagMatcher:
  def __init__(self, globalflags=None, globalnegflags=None, chanflags=None, channegflags=None, botflags=None, botnegflags=None, requireall=False):
    self.globalflags = globalflags
    self.globalnegflags = globalnegflags
    self.chanflags = chanflags
    self.channegflags = channegflags
    self.botflags = botflags
    self.botnegflags = botnegflags
    self.requireall = requireall
  
  @staticmethod
  def flagcheck(posflags, negflags, requireall, flags):
    if not requireall:
      if posflags and not posflags & flags:
        return False
    else:
      if posflags and not posflags & flags == posflags:
        return False
    if negflags and negflags & flags:
      return False
    return True

  def match(self, flags : FlagRecord):
    if not FlagMatcher.flagcheck(self.globalflags, self.globalnegflags, self.requireall, flags.globl):
      return False
    if not FlagMatcher.flagcheck(self.chanflags, self.channegflags, self.requireall, flags.chan):
      return False
    if not FlagMatcher.flagcheck(self.botflags, self.botnegflags, self.requireall, flags.bot):
      return False
    return True
  
  def __repr__(self):
    return repr({
      'globalmatch': repr(self.globalflags),
      'globalnegmatch': repr(self.globalnegflags),
      'chanmatch': repr(self.chanflags),
      'channegmatch': repr(self.channegflags),
      'botmatch': repr(self.botflags),
      'botnegmatch': repr(self.botnegflags),
      'requireall': repr(self.requireall)
     })

class BindType(dict):
#  def __init__(self):
  def add(self, callback, flags, mask):
    self.update({uuid.uuid4().hex[:8]:{"callback":callback, "flags":flags, "mask":mask}})
  def list(self):
    for i in self:
      print(self[i]) 

class Binds:
  def __init__(self):
    pubm = BindType()
    join = BindType()
  def list(self):
    for i in vars(self):
      ret = getattr(self, i).list()
      if ret:
        print(ret)


#class Binds:
#  def __init__(self):
#    self.bindlist = {"pub": [], "msgm": [], "pubm": [], "join": []}
#  def list(self, mask=None):
#    if not mask:
#      result = self.bindlist
#    else:
#      result = []
#      for i in self.bindlist[mask]:
#        result += self.bindlist[mask][i]
#    return result
    #  def prettylist(self, mask=None):
#    if not mask:
#      for bindtype in self.bindlist:
#        for bindentry in self.bindlist[bindtype]:
#          print("      "+bindtype+" "+str(bindentry['flags'])+"     "+bindentry['cmd'])
#    return
#  def add(self, bindtype, cmd, flags, mask, callback):
#    self.bindlist[bindtype].append({"cmd" : cmd, "callback" : callback, "mask" : mask, "flags" : flags})
#    return

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

def on_pub(flags, nick, user, hand, chan, text):
  print("pub bind triggered with "+nick+" "+user+" "+hand+" "+chan+" "+text)
  print(__allbinds.bindlist["pub"])
  for i in __allbinds.bindlist["pub"]:
    if i["flags"].match(flags) and (i["cmd"] == text.split()[0]):
      print("flagmatcher {} matches flag record {}".format(repr(i["flags"]), repr(flags)))
      i["callback"](nick, user, hand, chan, text.split(" ", 1)[1])
    else:
      print("flagmatcher {} does not match flag record {}".format(repr(i["flags"]), repr(flags)))
  return

def on_pubm(flags, nick, user, hand, chan, text):
  pprint(flags)
  print("pubm bind triggered with "+nick+" "+user+" "+hand+" "+chan+" "+text)
  for i in __allbinds.bindlist["pubm"]:
    print("mask is "+i["mask"])
    if i["flags"].match(flags) and bindmask2re(i["mask"]).match(text):
      print("flagmatcher {} matches flag record {}".format(repr(i["flags"]), repr(flags)))
      i["callback"](nick, user, hand, chan, text)
    else:
      print("flagmatcher {} does not match flag record {}".format(repr(i["flags"]), repr(flags)))
  on_pub(flags, nick, user, hand, chan, text)
  return

def on_msgm(nick, user, hand, text):
  print("msgm bind triggered with "+" ".join([nick, user, hand, text]))
  for i in __allbinds.bindlist["pubm"]:
    print("mask is "+__allbinds.bindlist["msgm"][i]["mask"])
    __allbinds.bindlist["msgm"][i]["callback"](nick, user, hand, text)
  return

def on_join(flags, nick, user, hand, chan):
  """This method searches for binds to be triggered when a user joins a channel.

        :param mask: a user hostmask, wildcards supported
        :param flags: flags for the user

        :returns: nick hostmask handle channel
  """ 
  print("-={ join bind triggered")
  for i in __allbinds.bindlist["join"]:
    if i["flags"].match(flags) and bindmask2re(i["mask"]).match(text):
      print("flagmatcher {} matches flag record {}".format(repr(i["flags"]), repr(flags)))
      i["callback"](nick, user, hand, chan, text)
    else:
      print("flagmatcher {} does not match flag record {}".format(repr(i["flags"]), repr(flags)))
  on_pub(flags, nick, user, hand, chan, text)


def on_event(bindtype, globalflags, chanflags, botflags, *args):
  flags = FlagRecord(globalflags, chanflags, botflags)
  for arg in args:
    print(arg)
  if bindtype == "pub":
    on_pub(flags, *args);
  if bindtype == "pubm":
    on_pubm(flags, *args)
  elif bindtype == "msgm":
    on_msgm(flags, *args)
#  py.putserv("PRIVMSG :"+chan+" you are "+nick+" and you said "+text)
  return 0

__allbinds = Binds()
def add(bindtype, cmd, flags, mask, callback):
    __allbinds.add(bindtype, cmd, flags, mask, callback)
def list(mask=None):
    return __allbinds.list(mask)
