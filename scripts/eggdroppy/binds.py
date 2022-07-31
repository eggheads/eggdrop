import re
import uuid
import sys
from pprint import pprint

class BindType:
#  def __init__(self):
  def __init__(self, bindtype):
    self.__bindtype = bindtype
    self.__binds = {}

  def add(self, callback, flags, mask):
    self.__binds.update({uuid.uuid4().hex[:8]:{"callback":callback, "flags":flags, "mask":mask}})

  def list(self):
    return self.__binds

  def __repr__(self):
    return f"Bindtype {self.__bindtype}: {repr(self.__binds)}"

class Binds:
  def __init__(self):
    self.__binds = {}
    self.__binds["pubm"] = BindType("pubm")
    self.__binds["join"] = BindType("join")

  def __getattr__(self, name):
    return self.__binds[name]

  def all(self):
    return self.__binds

  def types(self):
    return self.__binds.keys()

  def __repr__(self):
    return repr(self.__binds)

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
  print(repr(__allbinds.pub.all()))
  for i in __allbinds.pub:
    if i["flags"].match(flags) and (i["cmd"] == text.split()[0]):
      print("flagmatcher {} matches flag record {}".format(repr(i["flags"]), repr(flags)))
      i["callback"](nick, user, hand, chan, text.split(" ", 1)[1])
    else:
      print("flagmatcher {} does not match flag record {}".format(repr(i["flags"]), repr(flags)))
  return

def on_pubm(flags, nick, user, hand, chan, text):
  pprint(flags)
  print("pubm bind triggered with "+nick+" "+user+" "+hand+" "+chan+" "+text)
  for i in __allbinds.pubm:
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
  for i in __allbinds.pubm:
    print("mask is "+__allbinds.msgm[i]["mask"])
    __allbinds.msgm[i]["callback"](nick, user, hand, text)
  return

def on_join(flags, nick, user, hand, chan):
  """This method searches for binds to be triggered when a user joins a channel.

        :param mask: a user hostmask, wildcards supported
        :param flags: flags for the user

        :returns: nick hostmask handle channel
  """ 
  print("-={ join bind triggered")
  for i in __allbinds.join:
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

for bindtype in __allbinds.types():
  setattr(sys.modules[__name__], bindtype, getattr(__allbinds, bindtype))

def all():
  return __allbinds.all()

def types():
  return __allbinds.types(i)
