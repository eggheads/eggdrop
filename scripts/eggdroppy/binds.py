from dataclasses import dataclass
from typing import Callable
import re
import uuid
import sys
from pprint import pprint, pformat
import eggdrop
from eggdroppy.flags import FlagRecord, FlagMatcher
from dataclasses import dataclass

empty_flags = FlagMatcher()

@dataclass
class Bind:
  flags: FlagMatcher
  mask: str
  callback: Callable
  hits: int = 0

  @property
  def id(self):
    return f'bind{hex(id(self))[2:]}'

class BindCallback:
  def __init__(self, callback : Callable):
    self.__callback = callback
  def __call__(self, *args, **kwargs):
    self.__callback(*args, **kwargs)
  def __str__(self):
    return self.__callback.__name__

class BindType:
  """ A BindType is an event that can trigger an Eggdrop response

  Each event that Eggdrop refers to, called a bind, requires a :class:`BindType` to be loaded by the
  :class:`Binds` class.

  Args:
    bindtype (string): A string representing one of the core Eggdrop bind types
    managed (bool): True if bindtype is managed by Eggdrop
  """
  def __init__(self, bindtype, managed):
    self.__bindtype = bindtype
    self.__managed = managed
    self.__binds = {}

  @staticmethod
  def make_callback_func(callback):
    return BindCallback(callback)

  def add(self, mask : str, callback : Callable, flags : FlagMatcher = empty_flags):
    """ Register a new bind event

    Adds a new :class:`BindType` attribute to a :class:`Bind` object.

    Args:
      flags (object): a flag object, we'll figure this out soon
      mask (str): mask or command or something, maybe find a better word here
      callback (method): The name of the function you wish to call when the event is triggered
    """
    cb = self.make_callback_func(callback)
    if flags is None:
      flags = FlagMatcher()
    bind = Bind(flags=flags, mask=mask, callback=cb)
    self.__binds[bind.id] = bind
    if self.__managed:
      eggdrop.bind(self.__bindtype, mask, cb)

  def delete(self, bindid):
    bind = self.__binds[bindid]
    if self.__managed:
      eggdrop.unbind(self.__bindtype, bind.mask, bind.callback)
    if bindid in self.__binds:
      del self.__binds[bindid]

  def list(self):
    """ List all binds of the ``bindtype``

    Returns:
      list: A list of binds, in the format {A B C D}
    """
    return self.__binds

  def all(self):
    return self.list()

  def __str__(self):
    return f"{self.__bindtype}-binds: {str(self.__binds)}"

class Binds:
  """ A :class:`Binds` object holds a collection of :class:`BindTypes` objects

    All binds that are added to Eggdrop are collected and accessed through a :class:`Binds` object. Each
    event type that Eggdrop reacts to is added to the :class:`Binds` object as a bind via a
    :class:`BindTypes` object.

    Args: None
  """
  def __init__(self):
    self.__binds = {}
    self.__binds["pubm"] = BindType("pubm", False)
    self.__binds["join"] = BindType("join", False)
    self.__binds["dcc"] = BindType("dcc", True)

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

  def __str__(self):
    return [{x: [str(b) for b in y]} for x, y in self.__binds.items()]

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
  return bind.flags.match(realflags)

def on_pub(flags, nick, uhost, hand, chan, text):
  print(f"on_pub({pformat(flags)}, {nick}, {uhost}, {hand}, {chan}, {text.split()[0]}, {text.split(' ', 1)[1]})")
  for b in __allbinds.pub.list().values():
    match_flags = flags_ok(b, flags)
    match_mask = (b.mask == text.split()[0])
    call_bind = True if match_flags and match_mask else False
    print(f"  checking against {str(b)}: Flags={match_flags} Mask={match_mask}: {'Trigger' if call_bind else 'Skip'}")
    if call_bind:
      b.hits += 1
      b.callback(nick, uhost, hand, chan, text.split(" ", 1)[1])
  return

def on_pubm(flags, nick, uhost, hand, chan, text):
  print(f"on_pubm({pformat(flags)}, {nick}, {uhost}, {hand}, {chan}, {text})")
  for b in __allbinds.pubm.list().values():
    match_flags = flags_ok(b, flags)
    match_mask = bool(bindmask2re(b.mask).match(text))
    call_bind = True if match_flags and match_mask else False
    print(f"  checking against {str(b)}: Flags={match_flags} Mask={match_mask}: {'Trigger' if call_bind else 'Skip'}")
    if call_bind:
      b.hits += 1
      b.callback(nick, uhost, hand, chan, text)
  on_pub(flags, nick, uhost, hand, chan, text)
  return

def on_msgm(flags, nick, uhost, hand, text):
  print(f"on_msgm({pformat(flags)}, {nick}, {uhost}, {hand}, {text})")
  for b in __allbinds.msgm.list().values():
    match_flags = flags_ok(b, flags)
    match_mask = bool(bindmask2re(b.mask).match(text))
    call_bind = True if match_flags and match_mask else False
    print(f"  checking against {str(b)}: Flags={match_flags} Mask={match_mask}: {'Trigger' if call_bind else 'Skip'}")
    if call_bind:
      b.hits += 1
      b.callback(nick, uhost, hand, text)
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
    match_mask = bool(bindmask2re(b.mask).match(f"{nick}!{uhost}"))
    call_bind = True if match_flags and match_mask else False
    print(f"  checking against {str(b)}: Flags={match_flags} Mask={match_mask}: {'Trigger' if call_bind else 'Skip'}")
    if call_bind:
      b.hits += 1
      b.callback(nick, uhost, hand, chan)

def on_event(bindtype, globalflags, chanflags, botflags, *args):
  flags = FlagRecord(globalflags, chanflags, botflags)
  print(f"on_event({bindtype}, {str(globalflags)}, {str(chanflags)}, {str(botflags)}, {pformat(args)}")
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
  return __allbinds.types()

def print_all(*args):
  print("{0: <8} | {1: <18} | {2: <12} | {3: <24} | {4}".format('ID', 'function', 'flags', 'mask', 'hits'))
  for i in types():
    if __allbinds.all()[i].all():
      print("-"*78)
      print(f'{i:<8}')
      print("-"*78)
      for j in __allbinds.all()[i].all().keys():
        print(f'{j} | {str(__allbinds.all()[i].all()[j].callback):<18} | {str(__allbinds.all()[i].all()[j].flags):<12} | {__allbinds.all()[i].all()[j].mask:<24} | {__allbinds.all()[i].all()[j].hits:<4}')
  print("-"*78)

__allbinds.dcc.add(mask='pybinds', callback=print_all)