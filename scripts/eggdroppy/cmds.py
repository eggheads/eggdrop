from enum import IntEnum
import eggdrop

class IRCQueue(IntEnum):
  NOQUEUE = 0
  SERVER = eggdrop.QUEUE_SERVER
  HELP = eggdrop.QUEUE_HELP
  QUICK = MODE = eggdrop.QUEUE_MODE

def ircsend(text, queue = IRCQueue.SERVER, make_first=False):
  if queue == IRCQueue.QUICK and make_first:
    queuenum = eggdrop.QUEUE_MODE_NEXT
  elif queue == IRCQueue.SERVER and make_first:
    queuenum = eggdrop.QUEUE_SERVER_NEXT
  elif queue == IRCQueue.HELP and make_first:
    queuenum = eggdrop.QUEUE_HELP_NEXT
  elif queue == IRCQueue.NOQUEUE:
    raise Exception("Not implemented yet")
  eggdrop.ircsend(text.rstrip('\r\n'), queue)

# unnecessary shorthands (?)
def putserv(text, make_first=False):
  ircsend(text, queue=IRCQueue.SERVER, make_first=make_first)

def puthelp(text, make_first=False):
  ircsend(text, queue=IRCQueue.HELP, make_first=make_first)

def putquick(text, make_first=False):
  ircsend(text, queue=IRCQueue.MODE, make_first=make_first)

# useful shorthands
def putmsg(dst, text):
  for line in text.split('\n'):
    ircsend(f"PRIVMSG {dst} :{line}")

def putnotc(dst, text):
  for line in text.split('\n'):
    ircsend(f"NOTICE {dst} :{line}")

