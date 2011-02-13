/*
 * botmsg.c -- handles:
 *   formatting of messages to be sent on the botnet
 *   sending differnet messages to different versioned bots
 *
 * by Darrin Smith (beldin@light.iinet.net.au)
 *
 * $Id: botmsg.c,v 1.40 2011/02/13 14:19:33 simple Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2011 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "main.h"
#include "tandem.h"

extern struct dcc_t *dcc;
extern int dcc_total, tands;
extern char botnetnick[];
extern party_t *party;
extern Tcl_Interp *interp;
extern struct userrec *userlist;

static char OBUF[1024];


#ifndef NO_OLD_BOTNET
/* Ditto for tandem bots
 */
void tandout_but EGG_VARARGS_DEF(int, arg1)
{
  int i, x, len;
  char *format;
  char s[601];
  va_list va;

  x = EGG_VARARGS_START(int, arg1, va);
  format = va_arg(va, char *);

  egg_vsnprintf(s, 511, format, va);
  va_end(va);
  s[sizeof(s) - 1] = 0;

  len = strlen(s);

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_BOT) && (i != x) && (b_numver(i) < NEAT_BOTNET))
      tputs(dcc[i].sock, s, len);
}
#endif

/* Thank you ircu :) */
static char tobase64array[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '[', ']'
};

char *int_to_base64(unsigned int val)
{
  static char buf_base64[12];
  int i = 11;

  buf_base64[11] = 0;
  if (!val) {
    buf_base64[10] = 'A';
    return buf_base64 + 10;
  }
  while (val) {
    i--;
    buf_base64[i] = tobase64array[val & 0x3f];
    val = val >> 6;
  }
  return buf_base64 + i;
}

char *int_to_base10(int val)
{
  static char buf_base10[17];
  int p = 0;
  int i = 16;

  buf_base10[16] = 0;
  if (!val) {
    buf_base10[15] = '0';
    return buf_base10 + 15;
  }
  if (val < 0) {
    p = 1;
    val *= -1;
  }
  while (val) {
    i--;
    buf_base10[i] = '0' + (val % 10);
    val /= 10;
  }
  if (p) {
    i--;
    buf_base10[i] = '-';
  }
  return buf_base10 + i;
}

char *unsigned_int_to_base10(unsigned int val)
{
  static char buf_base10[16];
  int i = 15;

  buf_base10[15] = 0;
  if (!val) {
    buf_base10[14] = '0';
    return buf_base10 + 14;
  }
  while (val) {
    i--;
    buf_base10[i] = '0' + (val % 10);
    val /= 10;
  }
  return buf_base10 + i;
}

int simple_sprintf EGG_VARARGS_DEF(char *, arg1)
{
  char *buf, *format, *s;
  int c = 0, i;
  va_list va;

  buf = EGG_VARARGS_START(char *, arg1, va);
  format = va_arg(va, char *);

  while (*format && c < 1023) {
    if (*format == '%') {
      format++;
      switch (*format) {
      case 's':
        s = va_arg(va, char *);

        break;
      case 'd':
      case 'i':
        i = va_arg(va, int);

        s = int_to_base10(i);
        break;
      case 'D':
        i = va_arg(va, int);

        s = int_to_base64((unsigned int) i);
        break;
      case 'u':
        i = va_arg(va, unsigned int);

        s = unsigned_int_to_base10(i);
        break;
      case '%':
        buf[c++] = *format++;
        continue;
      case 'c':
        buf[c++] = (char) va_arg(va, int);

        format++;
        continue;
      default:
        continue;
      }
      if (s)
        while (*s && c < 1023)
          buf[c++] = *s++;
      format++;
    } else
      buf[c++] = *format++;
  }
  va_end(va);
  buf[c] = 0;
  return c;
}

/* Ditto for tandem bots
 */
void send_tand_but(int x, char *buf, int len)
{
  int i, iso = 0;

  if (len < 0) {
    len = -len;
    iso = 1;
  }
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_BOT) && (i != x) &&
        (b_numver(i) >= NEAT_BOTNET) &&
        (!iso || !(bot_flags(dcc[i].user) & BOT_ISOLATE)))
      tputs(dcc[i].sock, buf, len);
}

void botnet_send_bye()
{
  if (tands > 0) {
    send_tand_but(-1, "bye\n", 4);
#ifndef NO_OLD_BOTNET
    tandout_but(-1, "bye\n");
#endif
  }
}

void botnet_send_chan(int idx, char *botnick, char *user, int chan, char *data)
{
  int i;

  if ((tands > 0) && (chan < GLOBAL_CHANS)) {
    if (user) {
      i = simple_sprintf(OBUF, "c %s@%s %D %s\n", user, botnick, chan, data);
    } else {
      i = simple_sprintf(OBUF, "c %s %D %s\n", botnick, chan, data);
    }
    send_tand_but(idx, OBUF, -i);
#ifndef NO_OLD_BOTNET
    tandout_but(idx, "chan %s%s%s %d %s\n", user ? user : "",
                user ? "@" : "", botnick, chan, data);
#endif
  }
}

void botnet_send_act(int idx, char *botnick, char *user, int chan, char *data)
{
  int i;

  if ((tands > 0) && (chan < GLOBAL_CHANS)) {
    if (user) {
      i = simple_sprintf(OBUF, "a %s@%s %D %s\n", user, botnick, chan, data);
    } else {
      i = simple_sprintf(OBUF, "a %s %D %s\n", botnick, chan, data);
    }
    send_tand_but(idx, OBUF, -i);
#ifndef NO_OLD_BOTNET
    tandout_but(idx, "actchan %s%s%s %d %s\n", user ? user : "",
                user ? "@" : "", botnick, chan, data);
#endif
  }
}

void botnet_send_chat(int idx, char *botnick, char *data)
{
  int i;

  if (tands > 0) {
    i = simple_sprintf(OBUF, "ct %s %s\n", botnick, data);
    send_tand_but(idx, OBUF, -i);
#ifndef NO_OLD_BOTNET
    tandout_but(idx, "chat %s %s\n", botnick, data);
#endif
  }
}

void botnet_send_ping(int idx)
{
#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    tputs(dcc[idx].sock, "ping\n", 5);
  else
#endif
    tputs(dcc[idx].sock, "pi\n", 3);
}

void botnet_send_pong(int idx)
{
#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    tputs(dcc[idx].sock, "pong\n", 5);
  else
#endif
    tputs(dcc[idx].sock, "po\n", 3);
}

void botnet_send_priv EGG_VARARGS_DEF(int, arg1)
{
  int idx, l;
  char *from, *to, *tobot, *format;
  char tbuf[1024];
  va_list va;

  idx = EGG_VARARGS_START(int, arg1, va);
  from = va_arg(va, char *);
  to = va_arg(va, char *);
  tobot = va_arg(va, char *);
  format = va_arg(va, char *);

  egg_vsnprintf(tbuf, 450, format, va);
  va_end(va);
  tbuf[sizeof(tbuf) - 1] = 0;

  if (tobot) {
#ifndef NO_OLD_BOTNET
    if (b_numver(idx) < NEAT_BOTNET)
      l = simple_sprintf(OBUF, "priv %s %s@%s %s\n", from, to, tobot, tbuf);
    else
#endif
      l = simple_sprintf(OBUF, "p %s %s@%s %s\n", from, to, tobot, tbuf);
  } else {
#ifndef NO_OLD_BOTNET
    if (b_numver(idx) < NEAT_BOTNET)
      l = simple_sprintf(OBUF, "priv %s %s %s\n", from, to, tbuf);
    else
#endif
      l = simple_sprintf(OBUF, "p %s %s %s\n", from, to, tbuf);
  }
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_who(int idx, char *from, char *to, int chan)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "who %s %s %d\n", from, to, chan);
  else
#endif
    l = simple_sprintf(OBUF, "w %s %s %D\n", from, to, chan);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_infoq(int idx, char *par)
{
  int i = simple_sprintf(OBUF, "i? %s\n", par);

  send_tand_but(idx, OBUF, i);
#ifndef NO_OLD_BOTNET
  tandout_but(idx, "info? %s\n", par);
#endif
}

void botnet_send_unlink(int idx, char *who, char *via, char *bot, char *reason)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "unlink %s %s %s %s\n", who, via, bot, reason);
  else
#endif
    l = simple_sprintf(OBUF, "ul %s %s %s %s\n", who, via, bot, reason);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_link(int idx, char *who, char *via, char *bot)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "link %s %s %s\n", who, via, bot);
  else
#endif
    l = simple_sprintf(OBUF, "l %s %s %s\n", who, via, bot);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_unlinked(int idx, char *bot, char *args)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "un %s %s\n", bot, args ? args : "");
    send_tand_but(idx, OBUF, l);
#ifndef NO_OLD_BOTNET
    if ((idx >= 0) && (b_numver(idx) >= NEAT_BOTNET) && args && args[0])
      tandout_but(idx, "chat %s %s\n", lastbot(bot), args);
    tandout_but(idx, "unlinked %s\n", bot);
#endif
  }
}

void botnet_send_nlinked(int idx, char *bot, char *next, char flag, int vernum)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "n %s %s %c%D\n", bot, next, flag, vernum);
    send_tand_but(idx, OBUF, l);
#ifndef NO_OLD_BOTNET
    if (flag == '!') {
      flag = '-';
      tandout_but(idx, "chat %s %s %s\n", next, NET_LINKEDTO, bot);
    }
    tandout_but(idx, "nlinked %s %s %c%d\n", bot, next, flag, vernum);
#endif
  }
}

void botnet_send_traced(int idx, char *bot, char *buf)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "traced %s %s\n", bot, buf);
  else
#endif
    l = simple_sprintf(OBUF, "td %s %s\n", bot, buf);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_trace(int idx, char *to, char *from, char *buf)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "trace %s %s %s:%s\n", to, from, buf, botnetnick);
  else
#endif
    l = simple_sprintf(OBUF, "t %s %s %s:%s\n", to, from, buf, botnetnick);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_update(int idx, tand_t *ptr)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "u %s %c%D\n", ptr->bot, ptr->share, ptr->ver);
    send_tand_but(idx, OBUF, l);
#ifndef NO_OLD_BOTNET
    tandout_but(idx, "update %s %c%d\n", ptr->bot, ptr->share, ptr->ver);
#endif
  }
}

void botnet_send_reject(int idx, char *fromp, char *frombot, char *top,
                        char *tobot, char *reason)
{
  int l;
  char to[NOTENAMELEN + 1], from[NOTENAMELEN + 1];

  if (!(bot_flags(dcc[idx].user) & BOT_ISOLATE)) {
    if (tobot) {
      simple_sprintf(to, "%s@%s", top, tobot);
      top = to;
    }
    if (frombot) {
      simple_sprintf(from, "%s@%s", fromp, frombot);
      fromp = from;
    }
    if (!reason)
      reason = "";
#ifndef NO_OLD_BOTNET
    if (b_numver(idx) < NEAT_BOTNET)
      l = simple_sprintf(OBUF, "reject %s %s %s\n", fromp, top, reason);
    else
#endif
      l = simple_sprintf(OBUF, "r %s %s %s\n", fromp, top, reason);
    tputs(dcc[idx].sock, OBUF, l);
  }
}

void botnet_send_zapf(int idx, char *a, char *b, char *c)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "zapf %s %s %s\n", a, b, c);
  else
#endif
    l = simple_sprintf(OBUF, "z %s %s %s\n", a, b, c);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_zapf_broad(int idx, char *a, char *b, char *c)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "zb %s %s%s%s\n", a, b ? b : "", b ? " " : "", c);
    send_tand_but(idx, OBUF, l);
#ifndef NO_OLD_BOTNET
    tandout_but(idx, "zapf-broad %s\n", OBUF + 3);
#endif
  }
}

void botnet_send_motd(int idx, char *from, char *to)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "motd %s %s\n", from, to);
  else
#endif
    l = simple_sprintf(OBUF, "m %s %s\n", from, to);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_filereject(int idx, char *path, char *from, char *reason)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "filereject %s %s %s\n", path, from, reason);
  else
#endif
    l = simple_sprintf(OBUF, "f! %s %s %s\n", path, from, reason);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_filesend(int idx, char *path, char *from, char *data)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "filesend %s %s %s\n", path, from, data);
  else
#endif
    l = simple_sprintf(OBUF, "fs %s %s %s\n", path, from, data);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_filereq(int idx, char *from, char *bot, char *path)
{
  int l;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET)
    l = simple_sprintf(OBUF, "filereq %s %s:%s\n", from, bot, path);
  else
#endif
    l = simple_sprintf(OBUF, "fr %s %s:%s\n", from, bot, path);
  tputs(dcc[idx].sock, OBUF, l);
}

void botnet_send_idle(int idx, char *bot, int sock, int idle, char *away)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "i %s %D %D %s\n", bot, sock, idle,
                       away ? away : "");
    send_tand_but(idx, OBUF, -l);
#ifndef NO_OLD_BOTNET
    if (away && away[0])
      tandout_but(idx, "away %s %d %s\n", bot, sock, away);
    tandout_but(idx, "idle %s %d %d\n", bot, sock, idle);
#endif
  }
}

void botnet_send_away(int idx, char *bot, int sock, char *msg, int linking)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "aw %s%s %D %s\n",
                       ((idx >= 0) && linking) ? "!" : "",
                       bot, sock, msg ? msg : "");
    send_tand_but(idx, OBUF, -l);
#ifndef NO_OLD_BOTNET
    if (msg) {
      if (idx < 0) {
        tandout_but(idx, "chan %s %d %s is now away: %s.\n", bot,
                    dcc[linking].u.chat->channel, dcc[linking].nick, msg);
      } else if ((b_numver(idx) >= NEAT_BOTNET)) {
        int partyidx = getparty(bot, sock);

        if (partyidx >= 0)
          tandout_but(idx, "chan %s %d %s %s: %s.\n", bot,
                      party[partyidx].chan, party[partyidx].nick,
                      NET_AWAY, msg);
      }
      tandout_but(idx, "away %s %d %s\n", bot, sock, msg);
    } else {
      if (idx < 0) {
        tandout_but(idx, "chan %s %d %s %s.\n", bot,
                    dcc[linking].u.chat->channel, dcc[linking].nick,
                    NET_UNAWAY);
      } else if (b_numver(idx) >= NEAT_BOTNET) {
        int partyidx = getparty(bot, sock);

        if (partyidx >= 0)
          tandout_but(idx, "chan %s %d %s %s.\n", bot,
                      party[partyidx].chan, party[partyidx].nick, NET_UNAWAY);
      }
      tandout_but(idx, "unaway %s %d\n", bot, sock);
    }
#endif
  }
}

void botnet_send_join_idx(int useridx, int oldchan)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "j %s %s %D %c%D %s\n",
                       botnetnick, dcc[useridx].nick,
                       dcc[useridx].u.chat->channel, geticon(useridx),
                       dcc[useridx].sock, dcc[useridx].host);
    send_tand_but(-1, OBUF, -l);
#ifndef NO_OLD_BOTNET
    tandout_but(-1, "join %s %s %d %c%d %s\n", botnetnick,
                dcc[useridx].nick, dcc[useridx].u.chat->channel,
                geticon(useridx), dcc[useridx].sock, dcc[useridx].host);
    tandout_but(-1, "chan %s %d %s %s %s.\n",
                botnetnick, dcc[useridx].u.chat->channel,
                dcc[useridx].nick, NET_JOINEDTHE,
                dcc[useridx].u.chat->channel ? "channel" : "party line");
    if ((oldchan >= 0) && (oldchan < GLOBAL_CHANS)) {
      tandout_but(-1, "chan %s %d %s %s %s.\n",
                  botnetnick, oldchan,
                  dcc[useridx].nick, NET_LEFTTHE,
                  oldchan ? "channel" : "party line");
    }
#endif
  }
}

void botnet_send_join_party(int idx, int linking, int useridx, int oldchan)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "j %s%s %s %D %c%D %s\n", linking ? "!" : "",
                       party[useridx].bot, party[useridx].nick,
                       party[useridx].chan, party[useridx].flag,
                       party[useridx].sock,
                       party[useridx].from ? party[useridx].from : "");
    send_tand_but(idx, OBUF, -l);
#ifndef NO_OLD_BOTNET
    tandout_but(idx, "join %s %s %d %c%d %s\n", party[useridx].bot,
                party[useridx].nick, party[useridx].chan,
                party[useridx].flag, party[useridx].sock,
                party[useridx].from ? party[useridx].from : "");
    if ((idx < 0) || (!linking && (b_numver(idx) >= NEAT_BOTNET))) {
      tandout_but(idx, "chan %s %d %s %s %s.\n",
                  party[useridx].bot, party[useridx].chan,
                  party[useridx].nick, NET_JOINEDTHE,
                  party[useridx].chan ? "channel" : "party line");
    }
    if ((oldchan >= 0) && (oldchan < GLOBAL_CHANS) &&
        ((idx < 0) || (b_numver(idx) >= NEAT_BOTNET))) {
      tandout_but(idx, "chan %s %d %s %s %s.\n",
                  party[useridx].bot, oldchan, party[useridx].nick,
                  NET_LEFTTHE, party[useridx].chan ? "channel" : "party line");
    }
#endif
  }
}

void botnet_send_part_idx(int useridx, char *reason)
{
  int l = simple_sprintf(OBUF, "pt %s %s %D %s\n", botnetnick,
                         dcc[useridx].nick, dcc[useridx].sock,
                         reason ? reason : "");

  if (tands > 0) {
    send_tand_but(-1, OBUF, -l);
#ifndef NO_OLD_BOTNET
    tandout_but(-1, "part %s %s %d\n", botnetnick,
                dcc[useridx].nick, dcc[useridx].sock);
    tandout_but(-1, "chan %s %d %s has left the %s%s%s.\n",
                botnetnick, dcc[useridx].u.chat->channel,
                dcc[useridx].nick,
                dcc[useridx].u.chat->channel ? "channel" : "party line",
                reason ? ": " : "", reason ? reason : "");
#endif
  }
}

void botnet_send_part_party(int idx, int partyidx, char *reason, int silent)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "pt %s%s %s %D %s\n",
                       silent ? "!" : "", party[partyidx].bot,
                       party[partyidx].nick, party[partyidx].sock,
                       reason ? reason : "");
    send_tand_but(idx, OBUF, -l);
#ifndef NO_OLD_BOTNET
    tandout_but(idx, "part %s %s %d\n", party[partyidx].bot,
                party[partyidx].nick, party[partyidx].sock);
    if (((idx < 0) || (b_numver(idx) >= NEAT_BOTNET)) && !silent) {
      tandout_but(idx, "chan %s %d %s has left the %s%s%s.\n",
                  party[partyidx].bot, party[partyidx].chan,
                  party[partyidx].nick,
                  party[partyidx].chan ? "channel" : "party line",
                  reason ? ": " : "", reason ? reason : "");
    }
#endif
  }
}

void botnet_send_nkch(int useridx, char *oldnick)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "nc %s %D %s\n", botnetnick,
                       dcc[useridx].sock, dcc[useridx].nick);
    send_tand_but(-1, OBUF, -l);
#ifndef NO_OLD_BOTNET
    tandout_but(-1, "part %s %s %d\n", botnetnick,
                dcc[useridx].nick, dcc[useridx].sock);
    tandout_but(-1, "join %s %s %d %c%d %s\n", botnetnick,
                dcc[useridx].nick, dcc[useridx].u.chat->channel,
                geticon(useridx), dcc[useridx].sock, dcc[useridx].host);
    tandout_but(-1, "chan %s %d %s: %s -> %s.\n",
                botnetnick, dcc[useridx].u.chat->channel,
                oldnick, NET_NICKCHANGE, dcc[useridx].nick);
#endif
  }
}

void botnet_send_nkch_part(int butidx, int useridx, char *oldnick)
{
  int l;

  if (tands > 0) {
    l = simple_sprintf(OBUF, "nc %s %D %s\n", party[useridx].bot,
                       party[useridx].sock, party[useridx].nick);
    send_tand_but(butidx, OBUF, -l);
#ifndef NO_OLD_BOTNET
    tandout_but(butidx, "part %s %s %d\n", party[useridx].bot,
                party[useridx].nick, party[useridx].sock);
    tandout_but(butidx, "join %s %s %d %c%d %s\n", party[useridx].bot,
                party[useridx].nick, party[useridx].chan,
                party[useridx].flag, party[useridx].sock,
                party[useridx].from ? party[useridx].from : "");
    tandout_but(butidx, "chan %s %d %s : %s -> %s.\n",
                party[useridx].bot, party[useridx].chan,
                NET_NICKCHANGE, oldnick, party[useridx].nick);
#endif
  }
}

/* This part of add_note is more relevant to the botnet than
 * to the notes file.
 */
int add_note(char *to, char *from, char *msg, int idx, int echo)
{
  int status, i, iaway, sock;
  char *p, botf[81], ss[81], ssf[81];
  struct userrec *u;

  /* Notes have a length limit. Note + PRIVMSG header + nick + date must
   * be less than 512.
   */
  if (strlen(msg) > 450)
    msg[450] = 0;

  /* Is this a cross-bot note? If it is, 'to' will be of the format
   * 'user@bot'.
   */
  p = strchr(to, '@');
  if (p != NULL) {
    char x[21];

    *p = 0;
    strncpy(x, to, 20);
    x[20] = 0;
    *p = '@';
    p++;

    if (!egg_strcasecmp(p, botnetnick)) /* To me?? */
      return add_note(x, from, msg, idx, echo); /* Start over, dimwit. */

    if (egg_strcasecmp(from, botnetnick)) {
      if (strlen(from) > 40)
        from[40] = 0;

      if (strchr(from, '@')) {
        strcpy(botf, from);
      } else
        sprintf(botf, "%s@%s", from, botnetnick);

    } else
      strcpy(botf, botnetnick);

    i = nextbot(p);
    if (i < 0) {
      if (idx >= 0)
        dprintf(idx, BOT_NOTHERE);

      return NOTE_ERROR;
    }

    if (idx >= 0 && echo)
      dprintf(idx, "-> %s@%s: %s\n", x, p, msg);

    if (idx >= 0) {
      sprintf(ssf, "%lu:%s", dcc[idx].sock, botf);
      botnet_send_priv(i, ssf, x, p, "%s", msg);
    } else
      botnet_send_priv(i, botf, x, p, "%s", msg);

    return NOTE_OK;             /* Forwarded to the right bot */
  }

  /* Might be form "sock:nick" */
  splitc(ssf, from, ':');
  rmspace(ssf);
  splitc(ss, to, ':');
  rmspace(ss);
  if (!ss[0])
    sock = -1;
  else
    sock = atoi(ss);

  /* Don't process if there's a note binding for it */
  if (idx != -2) {            /* Notes from bots don't trigger it */
    if (check_tcl_note(from, to, msg)) {
      if (idx >= 0 && echo)
        dprintf(idx, "-> %s: %s\n", to, msg);

      return NOTE_TCL;
    }
  }

  /* Valid user? */
  u = get_user_by_handle(userlist, to);
  if (!u) {
    if (idx >= 0)
      dprintf(idx, USERF_UNKNOWN);

    return NOTE_ERROR;
  }

  /* Is the note to a bot? */
  if (is_bot(u)) {
    if (idx >= 0)
      dprintf(idx, BOT_NONOTES);

    return NOTE_ERROR;
  }

  /* Is user rejecting notes from this source? */
  if (match_noterej(u, from)) {
    if (idx >= 0)
      dprintf(idx, "%s rejected your note.\n", u->handle);

    return NOTE_REJECT;
  }

  status = NOTE_STORED;
  iaway = 0;

  /* Online right now? */
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type->flags & DCT_GETNOTES) &&
        (sock == -1 || sock == dcc[i].sock) &&
        !egg_strcasecmp(dcc[i].nick, to)) {
      int aok = 1;

      if (dcc[i].type == &DCC_CHAT) {

        /* Only check away if it's not from a bot. */
        if (dcc[i].u.chat->away != NULL && idx != -2) {
          aok = 0;

          if (idx >= 0)
            dprintf(idx, "%s %s: %s\n", dcc[i].nick, BOT_USERAWAY,
                    dcc[i].u.chat->away);

          if (!iaway)
            iaway = i;
          status = NOTE_AWAY;
        }
      }

      if (aok) {
        char *p, *fr = from, work[1024];
        int l = 0;

        while (*msg == '<' || *msg == '>') {
          p = newsplit(&msg);

          if (*p == '<')
            l += simple_sprintf(work + l, "via %s, ", p + 1);
          else if (*from == '@')
            fr = p + 1;
        }

        if (idx == -2 || !egg_strcasecmp(from, botnetnick))
          dprintf(i, "*** [%s] %s%s\n", fr, l ? work : "", msg);
        else
          dprintf(i, "%cNote [%s]: %s%s\n", 7, fr, l ? work : "", msg);

        if (idx >= 0 && echo)
          dprintf(idx, "-> %s: %s\n", to, msg);

        return NOTE_OK;
      }
    }
  }

  if (idx == -2)
    return NOTE_OK; /* Error msg from a tandembot: don't store. */

  /* Call 'storenote' Tcl command. */
  simple_sprintf(ss, "%d", (idx >= 0) ? dcc[idx].sock : -1);
  Tcl_SetVar(interp, "_from", from, 0);
  Tcl_SetVar(interp, "_to",   to,   0);
  Tcl_SetVar(interp, "_data", msg,  0);
  Tcl_SetVar(interp, "_idx",  ss,   0);
  if (Tcl_VarEval(interp, "storenote", " $_from $_to $_data $_idx", NULL) ==
      TCL_OK) {

    if (!tcl_resultempty())
      status = NOTE_FWD;

    /* User is away in all sessions -- just notify the user that a
     * message arrived and was stored (only oldest session is notified).
     */
    if (status == NOTE_AWAY)
      dprintf(iaway, "*** %s.\n", BOT_NOTEARRIVED);

    return status;
  }

  /* If we haven't returned anything else by now, assume an error occurred. */
  return NOTE_ERROR;
}
