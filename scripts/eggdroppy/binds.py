import re
import uuid
import sys
from pprint import pprint, pformat
from eggdroppy import FlagRecord

class BindType:
  """ A BindType is an event that can trigger an Eggdrop response

  Each event that Eggdrop refers to, called a bind, requires a :class:`BindType` to be loaded by the
  :class:`Binds` class.

  Args:
    bindtype (string): A string representing one of the core Eggdrop bind types
  """
  def __init__(self, bindtype):
    self.__bindtype = bindtype
    self.__binds = {}

  def add(self, callback, flags, mask):
    """ Register a new bind event

    Adds a new :class:`BindType` attribute to a :class:`Bind` object.

    Args:
      callback (method): The name of the function you wish to call when the event is triggered
      flags (object): a flag object, we'll figure this out soon
      mask (str): mask or command or something, maybe find a better word here
    """
    self.__binds.update({uuid.uuid4().hex[:8]:{"callback":callback, "flags":flags, "mask":mask, "hits":0}})

  def list(self):
    """ List all binds of the ``bindtype``

    Returns:
      list: A list of binds, in the format {A B C D}
    """
    return self.__binds

  def all(self):
    return self.list()

  def __repr__(self):
    return f"Bindtype {self.__bindtype}: {repr(self.__binds)}"

class Binds:
  """ A :class:`Binds` object holds a collection of :class:`BindTypes` objects

    All binds that are added to Eggdrop are collected and accessed through a :class:`Binds` object. Each
    event type that Eggdrop reacts to is added to the :class:`Binds` object as a bind via a
    :class:`BindTypes` object.

    Args: None
  """
  def __init__(self):
    self.__binds = {}
    self.__binds["pubm"] = BindType("pubm")
    self.__binds["join"] = BindType("join")

  def __getattr__(self, name):
    return self.__binds[name]

  def all(self):
    """ Lists all binds registered with the object

    Returns:
      list: A list, or maybe a dict? of all binds
    """
    return self.__binds

  def types(self):
    """ Lists all :class:`BindType` attributes added to the :class:`Binds` object

    Returns:
      list: maybe a list? of attributes
    """
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

def flags_ok(bind, realflags):
  return bind["flags"].match(realflags)

def on_pub(flags, nick, uhost, hand, chan, text):
  print(f"on_pub({pformat(flags)}, {nick}, {uhost}, {hand}, {chan}, {text.split()[0]}, {text.split(' ', 1)[1]})")
  for b in __allbinds.pub.list().values():
    match_flags = flags_ok(b, flags)
    match_mask = (b["cmd"] == text.split()[0])
    call_bind = True if match_flags and match_mask else False
    print(f"  checking against {repr(b)}: Flags={match_flags} Mask={match_mask}: {'Trigger' if call_bind else 'Skip'}")
    if call_bind:
      b["hits"] += 1
      b["callback"](nick, uhost, hand, chan, text.split(" ", 1)[1])
  return

def on_pubm(flags, nick, uhost, hand, chan, text):
  print(f"on_pubm({pformat(flags)}, {nick}, {uhost}, {hand}, {chan}, {text})")
  for b in __allbinds.pubm.list().values():
    match_flags = flags_ok(b, flags)
    match_mask = bool(bindmask2re(b["mask"]).match(text))
    call_bind = True if match_flags and match_mask else False
    print(f"  checking against {repr(b)}: Flags={match_flags} Mask={match_mask}: {'Trigger' if call_bind else 'Skip'}")
    if call_bind:
      b["hits"] += 1
      b["callback"](nick, uhost, hand, chan, text)
  on_pub(flags, nick, uhost, hand, chan, text)
  return

def on_msgm(nick, uhost, hand, text):
  print(f"on_pubm({pformat(flags)}, {nick}, {uhost}, {hand}, {text})")
  for b in __allbinds.msgm.list().values():
    match_flags = flags_ok(b, flags)
    match_mask = bool(bindmask2re(b["mask"]).match(text))
    call_bind = True if match_flags and match_mask else False
    print(f"  checking against {repr(b)}: Flags={match_flags} Mask={match_mask}: {'Trigger' if call_bind else 'Skip'}")
    if call_bind:
      b["hits"] += 1
      b["callback"](nick, uhost, hand, text)
  return

def on_join(flags, nick, uhost, hand, chan):
  """This method searches for binds to be triggered when a user joins a channel.

        :param mask: a user hostmask, wildcards supported
        :param flags: flags for the user

        :returns: nick hostmask handle channel
  """ 
  print(f"on_join({pformat(flags)}, {nick}, {uhost}, {hand}, {chan})")
  for b in __allbinds.join.list().values():
    match_flags = flags_ok(b, flags)
    match_mask = bool(bindmask2re(b["mask"]).match(f"{nick}!{uhost}"))
    call_bind = True if match_flags and match_mask else False
    print(f"  checking against {repr(b)}: Flags={match_flags} Mask={match_mask}: {'Trigger' if call_bind else 'Skip'}")
    if call_bind:
      b["hits"] += 1
      b["callback"](nick, uhost, hand, chan)

def on_event(bindtype, globalflags, chanflags, botflags, *args):
  flags = FlagRecord(globalflags, chanflags, botflags)
  print(f"on_event({bindtype}, {repr(globalflags)}, {repr(chanflags)}, {repr(botflags)}, {pformat(args)}")
  if bindtype == "pubm":
    on_pubm(flags, *args)
  elif bindtype == "msgm":
    on_msgm(flags, *args)
  elif bindtype == "join":
    on_join(flags, *args)
  return 0

__allbinds = Binds()

for bindtype in __allbinds.types():
  setattr(sys.modules[__name__], bindtype, getattr(__allbinds, bindtype))

def all():
  return __allbinds.all()

def types():
  return __allbinds.types(i)
