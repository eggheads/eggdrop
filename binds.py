#import py


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

  def add(self, bindtype, cmd, callback):
    self.bindlist[bindtype][cmd] = callback
    return

my_binds = Binds()
eggdrop = Eggdrop(binds=my_binds)


def myfunc(nick, user, hand, chan, text):
  print("Holy shit this worked- "+nick+" on "+chan+" said "+text)
  return

def on_pubm(nick, user, hand, chan, text):
  global eggdrop
  print("pubm bind triggered with "+nick+" "+user+" "+hand+" "+chan+" "+text)
  for i in eggdrop.binds.bindlist["pubm"]:
    eggdrop.binds.bindlist["pubm"][i](nick, user, hand, chan, text)
  return

def on_msgm(nick, user, hand, text):
  print("msgm bind triggered with "+" ".join([nick, user, hand, chan, text]))
  return

### add, delete, list

def on_event(bindtype, *args):
  for arg in args:
    print(arg)
  if bindtype == "pubm":
    on_pubm(*args)
  elif bindtype == "msgm":
    on_msgm(*args)
#  py.putserv("PRIVMSG :"+chan+" you are "+nick+" and you said "+text)
  return 0

eggdrop.binds.add("pubm", "!hi", myfunc)
