from enum import IntFlag
from pprint import pprint
import eggdrop

# TODO: sort these properly
class UserFlags(IntFlag):
  n = owner = eggdrop.USER_OWNER
  m = master = eggdrop.USER_MASTER
  a = autoop = eggdrop.USER_AUTOOP
  o = op = eggdrop.USER_OP
  g = autovoice = eggdrop.USER_GVOICE
  v = voice = eggdrop.USER_VOICE
  b = bot = eggdrop.USER_BOT
  c = common = eggdrop.USER_COMMON
  d = deop = eggdrop.USER_DEOP
  e = exempt = eggdrop.USER_EXEMPT
  f = friend = eggdrop.USER_FRIEND
  h = highlight = eggdrop.USER_HIGHLITE
  j = janitor = eggdrop.USER_JANITOR
  k = autokick = eggdrop.USER_KICK
  l = halfop = eggdrop.USER_HALFOP
  p = partyline = eggdrop.USER_PARTY
  q = devoice = eggdrop.USER_QUIET
  r = dehalfop = eggdrop.USER_DEHALFOP
  t = botnetmaster = eggdrop.USER_BOTMAST
  u = unshared = eggdrop.USER_UNSHARED
  w = wasoptest = eggdrop.USER_WASOPTEST
  x = xfer = eggdrop.USER_XFER
  y = autohalfop = eggdrop.USER_AUTOHALFOP
  z = washalfoptest = eggdrop.USER_WASHALFOPTEST

  def __repr__(self):
    if self.value == 0:
      return "-"
    return ''.join(f.name for f in self.__class__ if f.value & self.value)

class FlagRecord:
  def __init__(self, globalflags=None, chanflags=None, botflags=None):
    self.globl = UserFlags(globalflags)
    self.chan = UserFlags(chanflags)
    self.bot = UserFlags(botflags)
  
  def __str__(self):
    return 'globl: {}, chan: {}, bot: {}'.format(str(self.globl), str(self.chan), str(self.bot))

  def __repr__(self):
    return f"{repr(self.globl)}|{repr(self.chan)}|{repr(self.bot)}"

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
  def reprflags(pls, mns):
    s = ""
    if pls:
      s += "+" + repr(pls)
    if mns:
      s += "-" + repr(mns)
    return s

  def __repr__(self):
    s = ""
    sep = "&" if self.requireall else "|"
    if not self.globalflags and not self.globalnegflags and not self.chanflags and not self.channegflags and not self.botflags and not self.botnegflags:
      return "-"
    if self.globalflags or self.globalnegflags:
      s += self.reprflags(self.globalflags, self.globalnegflags)
    s += sep
    if self.chanflags or self.channegflags:
      s += self.reprflags(self.chanflags, self.channegflags)
    if self.botflags or self.botnegflags:
      s += sep
      s += self.reprflags(self.botflags, self.botnegflags)
    return s

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

