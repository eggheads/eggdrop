/* 
 * seen.c   - Implement the seen.tcl script functionality via module.
 *
 *            by ButchBub - Scott G. Taylor (staylor@mrynet.com) 
 *
 *      REQUIRED: Eggdrop Module version 1.2.0
 *
 *      0.1     1997-07-29      Initial. [BB]
 *      1.0     1997-07-31      Release. [BB]
 *      1.1     1997-08-05      Add nick->handle lookup for NICK's. [BB]
 *      1.2     1997-08-20      Minor fixes. [BB]
 *      1.2a    1997-08-24      Minor fixes. [BB]
 *
 */

/* 
 *  Currently, PUB, DCC and MSG commands are supported.  No party-line
 *      filtering is performed.
 *
 *  For boyfriend/girlfriend support, this module relies on the XTRA
 *      fields in the userfile to use BF and GF, respectively, for
 *      these fields.  
 *
 *  userinfo1.0.tcl nicely compliments this script by providing 
 *      the necessary commands to facilitate modification of these
 *      fields via DCC and IRC MSG commands.
 *
 *  A basic definition of the parsing syntax follows:
 *
 *      trigger :: seen [ <key> [ [ and | or ] <key> [...]]]
 *
 *        <key> :: <keyword> [ <keyarg> ]
 *
 *    <keyword> :: god | jesus | shit | me | yourself | my | <nick>'s |
 *                 your
 *       <nick> :: (any current on-channel IRC nick or userlist nick or handle)
 *
 *     <keyarg> :: (see below)
 *
 *              KEYWORD KEYARG
 *
 *              my      boyfriend
 *                      bf
 *                      girlfriend
 *                      gf
 *              your    owner
 *                      admin
 *                      (other)
 *              NICK's  boyfriend   
 *                      bf
 *                      girlfriend
 *                      gf
 *
 */

#define MAKING_SEEN
#define MODULE_NAME "seen"

#include <time.h>

#include "../module.h"
#include "../../users.h"
#include "../../chan.h"
#include "../channels.mod/channels.h"

static Function *global = NULL;
static void wordshift();
static void do_seen();
static char *match_trigger();
static char *getxtra();
static char *fixnick();

typedef struct {
  char *key;
  char *text;
} trig_data;

static trig_data trigdata[] =
{
  {"god", "Let's not get into a religious discussion, %s"},
  {"jesus", "Let's not get into a religious discussion, %s"},
  {"shit", "Here's looking at you, %s"},
  {"yourself", "Yeah, whenever I look in a mirror..."},
  {NULL, "You found me, %s!"},
  {"elvis", "Last time I was on the moon man."},
  {0, 0}
};

static int seen_expmem()
{
  int size = 0;

  return size;
}

/* PUB `seen' trigger. */
static int pub_seen(char *nick, char *host, char *hand,
		    char *channel, char *text)
{
  char prefix[50];
  struct chanset_t *chan = findchan(channel);

  context;
  if ((chan != NULL) && channel_seen(chan)) {
    sprintf(prefix, "PRIVMSG %s :", channel);
    do_seen(DP_HELP, prefix, nick, hand, channel, text);
  }
  return 0;
}

static int msg_seen(char *nick, char *host, struct userrec *u, char *text)
{
  char prefix[50];

  context;
  if (!u) {
    putlog(LOG_MISC, "*", "[%s!%s] seen %s", nick, host, text);
    return 0;
  }
  putlog(LOG_MISC, "*", "(%s!%s) !%s! SEEN %s", nick, host, u->handle, text);
  sprintf(prefix, "PRIVMSG %s :", nick);
  do_seen(DP_SERVER, prefix, nick, u->handle, "", text);
  return 0;
}

static int dcc_seen(struct userrec *u, int idx, char *par)
{
  context;
  putlog(LOG_CMDS, "*", "#%s# seen %s", dcc[idx].nick, par);
  do_seen(idx, "", dcc[idx].nick, dcc[idx].nick, "", par);
  return 0;
}

static void do_seen(int idx, char *prefix, char *nick, char *hand, char *channel,
		    char *text)
{
  char stuff[512];
  char word1[512], word2[512];
  char whotarget[512];
  char object[512];
  char *oix;
  char whoredirect[512];
  struct userrec *urec;
  struct chanset_t *chan;
  struct laston_info *li;
  struct chanuserrec *cr;
  memberlist *m;
  int onchan = 0;
  int i;
  time_t laston = 0;
  time_t work;
  char *lastonplace = 0;

  whotarget[0] = 0;
  whoredirect[0] = 0;
  object[0] = 0;
  /* Was ANYONE specified */
  if (!text[0]) {
    sprintf(stuff,
	    "%sUm, %s, it might help if you ask me about _someone_...\n",
	    prefix, nick);
    dprintf(idx, stuff);
    return;
  }
  wordshift(word1, text);
  oix = strchr(word1, '\'');
  /* Have we got a NICK's target? */
  if (oix == word1)
    return;			/* Skip anything starting with ' */
  context;
  if (oix && *oix &&
      ((oix[1] && (oix[1] == 's' || oix[1] == 'S') && !oix[2]) ||
       (!oix[1] &&
	(oix[-1] == 's' || oix[-1] == 'z' || oix[-1] == 'x' ||
	 oix[-1] == 'S' || oix[-1] == 'Z' || oix[-1] == 'X')))) {
    strncpy(object, word1, oix - word1);
    object[oix - word1] = 0;
    wordshift(word1, text);
    if (!word1[0]) {
      dprintf(idx, "%s%s's what, %s?\n", prefix, object, nick);
      return;
    }
    urec = get_user_by_handle(userlist, object);
    if (!urec) {
      chan = chanset;
      while (chan) {
	onchan = 0;
	m = ismember(chan, object);
	if (m) {
	  onchan = 1;
	  sprintf(stuff, "%s!%s", object, m->userhost);
	  urec = get_user_by_host(stuff);
	  if (!urec || !strcasecmp(object, urec->handle))
	    break;
	  strcat(whoredirect, object);
	  strcat(whoredirect, " is ");
	  strcat(whoredirect, urec->handle);
	  strcat(whoredirect, ", and ");
	  strcpy(object, urec->handle);
	  break;
	}
	chan = chan->next;
      }
      if (!onchan) {
	dprintf(idx, "%sI don't think I know who %s is, %s.\n",
		prefix, object, nick);
	return;
      }
    }
    if (!strcasecmp(word1, "bf") || !strcasecmp(word1, "boyfriend")) {
      strcpy(whotarget, getxtra(object, "BF"));
      if (whotarget[0]) {
	sprintf(whoredirect, "%s boyfriend is %s, and ",
		fixnick(object), whotarget);
	goto TARGETCONT;
      }
      dprintf(idx,
	      "%sI don't know who %s boyfriend is, %s.\n",
	      prefix, fixnick(object), nick);
      return;
    }
    if (!strcasecmp(word1, "gf") || !strcasecmp(word1, "girlfriend")) {
      strcpy(whotarget, getxtra(object, "GF"));
      if (whotarget[0]) {
	sprintf(whoredirect, "%s girlfriend is %s, and ",
		fixnick(object), whotarget);
	goto TARGETCONT;
      }
      dprintf(idx,
	      "%sI don't know who %s girlfriend is, %s.\n",
	      prefix, fixnick(object), nick);
      return;
    }
    dprintf(idx,
	  "%sWhy are you bothering me with questions about %s %s, %s?\n",
	    prefix, fixnick(object), word1, nick);
    return;
  }
  /* Keyword "my" */
  if (!strcasecmp(word1, "my")) {
    wordshift(word1, text);
    if (!word1[0]) {
      dprintf(idx, "%sYour what, %s?\n", prefix, nick);
      return;
    }
    /* Do I even KNOW the requestor? */
    if (hand[0] == '*' || !hand[0]) {
      dprintf(idx,
	      "%sI don't know you, %s, so I don't know about your %s.\n",
	      prefix, nick, word1);
      return;
    }
    /* "my boyfriend" */
    if (!strcasecmp(word1, "boyfriend") || !strcasecmp(word1, "bf")) {
      strcpy(whotarget, getxtra(hand, "BF"));
      if (whotarget[0]) {
	sprintf(whoredirect, "%s, your boyfriend is %s, and ",
		nick, whotarget);
      } else {
	dprintf(idx,
		"%sI didn't know you had a boyfriend, %s\n",
		prefix, nick);
	return;
      }
    }
    /* "my girlfriend" */
    else if (!strcasecmp(word1, "girlfriend") || !strcasecmp(word1, "gf")) {
      strcpy(whotarget, getxtra(hand, "GF"));
      if (whotarget[0]) {
	sprintf(whoredirect, "%s, your girlfriend is %s, and ",
		nick, whotarget);
      } else {
	dprintf(idx,
		"%sI didn't know you had a girlfriend, %s\n",
		prefix, nick);
	return;
      }
    } else {
      dprintf(idx,
	      "%sI don't know anything about your %s, %s.\n",
	      prefix, word1, nick);
      return;
    }
  }
  /* "your" keyword */
  else if (!strcasecmp(word1, "your")) {
    wordshift(word1, text);
    /* "your admin" */
    if (!strcasecmp(word1, "owner") || !strcasecmp(word1, "admin")) {
      if (admin[0]) {
	strcpy(word2, admin);
	wordshift(whotarget, word2);
	strcat(whoredirect, "My owner is ");
	strcat(whoredirect, whotarget);
	strcat(whoredirect, ", and ");
	if (!strcasecmp(whotarget, hand)) {
	  strcat(whoredirect, "that's YOU");
	  if (!strcasecmp(hand, nick)) {
	    strcat(whoredirect, "!!!");
	  } else {
	    strcat(whoredirect, ", ");
	    strcat(whoredirect, nick);
	    strcat(whoredirect, "!");
	  }
	  dprintf(idx, "%s%s\n", prefix, whoredirect);
	  return;
	}
      } else {			/* owner variable munged or not set */
	dprintf(idx,
		"%sI don't seem to recall who my owner is right now...\n",
		prefix);
	return;
      }
    } else {			/* no "your" target specified */
      dprintf(idx, "%sLet's not get personal, %s.\n",
	      prefix, nick);
      return;
    }
  }
  /* Check for keyword match in the internal table */
  else if (match_trigger(word1)) {
    sprintf(word2, "%s%s\n", prefix, match_trigger(word1));
    dprintf(idx, word2, nick);
    return;
  }
  /* Otherwise, make the target to the first word and continue */
  else {
    strcpy(whotarget, word1);
  }
  TARGETCONT:
  context;
  /* Looking for ones own nick? */
  if (!rfc_casecmp(nick, whotarget)) {
    dprintf(idx, "%s%sLooking for yourself, eh %s?\n",
	    prefix, whoredirect, nick);
    return;
  }
  /* Check if nick is on a channel */
  chan = chanset;
  while (chan) {
    m = ismember(chan, whotarget);
    if (m) {
      onchan = 1;
      sprintf(word1, "%s!%s", whotarget, m->userhost);
      urec = get_user_by_host(word1);
      if (!urec || !strcasecmp(whotarget, urec->handle))
	break;
      strcat(whoredirect, whotarget);
      strcat(whoredirect, " is ");
      strcat(whoredirect, urec->handle);
      strcat(whoredirect, ", and ");
      break;
    }
    chan = chan->next;
  }
  /* Check if nick is on a channel by xref'ing to handle */
  if (!onchan) {
    chan = chanset;
    while (chan) {
      m = chan->channel.member;
      while (m->nick[0]) {
	sprintf(word2, "%s!%s", m->nick, m->userhost);
	urec = get_user_by_host(word2);
	if (urec && !strcasecmp(urec->handle, whotarget)) {
	  onchan = 1;
	  strcat(whoredirect, whotarget);
	  strcat(whoredirect, " is ");
	  strcat(whoredirect, m->nick);
	  strcat(whoredirect, ", and ");
	  strcpy(whotarget, m->nick);
	  break;
	}
	m = m->next;
      }
      chan = chan->next;
    }
  }
  /* Check if the target was on the channel, but is netsplit */
  context;
  chan = findchan(channel);
  if (chan) {
    m = ismember(chan, whotarget);
    if (m && chan_issplit(m)) {
      dprintf(idx, "%s%s%s was just here, but got netsplit.\n",
	      prefix, whoredirect, whotarget);
      return;
    }
  }
  /* Check if the target IS on the channel */
  if (chan && m) {
    dprintf(idx, "%s%s%s is on the channel right now!\n",
	    prefix, whoredirect, whotarget);
    return;
  }
  /* Target not on this channel.   Check other channels */
  chan = chanset;
  while (chan) {
    m = ismember(chan, whotarget);
    if (m && chan_issplit(m)) {
      dprintf(idx,
	      "%s%s%s was just on %s, but got netsplit.\n",
	      prefix, whoredirect, whotarget, chan->name);
      return;
    }
    if (m) {
      dprintf(idx,
	      "%s%s%s is on %s right now!\n",
	      prefix, whoredirect, whotarget, chan->name);
      return;
    }
    chan = chan->next;
  }
  /* Target isn't on any of my channels. */
  /* See if target matches a handle in my userlist. */
  urec = get_user_by_handle(userlist, whotarget);
  /* No match, then bail out */
  if (!urec) {
    dprintf(idx, "%s%sI don't know who %s is.\n",
	    prefix, whoredirect, whotarget);
    return;
  }
  /* We had a userlist match to a handle */
  /* Is the target currently DCC CHAT to me on the botnet? */
  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type->flags & DCT_CHAT) {
      if (!strcasecmp(whotarget, dcc[i].nick)) {
	if (!rfc_casecmp(channel, dcc[i].u.chat->con_chan) &&
	    dcc[i].u.chat->con_flags & LOG_PUBLIC) {
	  strcat(whoredirect, whotarget);
	  strcat(whoredirect,
	   " is 'observing' this channel right now from my party line!");
	  dprintf(idx, "%s%s\n",
		  prefix, whoredirect);
	} else {
	  dprintf(idx,
		  "%s%s%s is linked to me via DCC CHAT right now!\n",
		  prefix, whoredirect, whotarget);
	}
	return;
      }
    }
  }
  /* Target known, but nowhere to be seen.  Give last IRC and botnet time */
  wordshift(word1, text);
  if (!strcasecmp(word1, "anywhere"))
    cr = NULL;
  else
    for (cr = urec->chanrec; cr; cr = cr->next) {
      if (!rfc_casecmp(cr->channel, channel)) {
	if (cr->laston) {
	  laston = cr->laston;
	  lastonplace = channel;
	  break;
	}
      }
    }
  if (!cr) {
    li = get_user(&USERENTRY_LASTON, urec);
    if (!li || !li->lastonplace || !li->lastonplace[0]) {
      dprintf(idx, "%s%sI've never seen %s around.\n",
	      prefix, whoredirect, whotarget);
      return;
    }
    lastonplace = li->lastonplace;
    laston = li->laston;
  }
  word1[0] = 0;
  word2[0] = 0;
  work = now - laston;
  if (work >= 86400) {
    sprintf(word2, "%lu day%s, ", work / 86400,
	    ((work / 86400) == 1) ? "" : "s");
    work = work % 86400;
  }
  if (work >= 3600) {
    sprintf(word2 + strlen(word2), "%lu hour%s, ", work / 3600,
	    ((work / 3600) == 1) ? "" : "s");
    work = work % 3600;
  }
  if (work >= 60) {
    sprintf(word2 + strlen(word2), "%lu minute%s, ", work / 60,
	    ((work / 60) == 1) ? "" : "s");
  }
  if (!word2[0] && (work < 60)) {
    strcpy(word2, "just moments ago!!");
  } else {
    strcpy(word2 + strlen(word2) - 2, " ago.");
  }
  if (lastonplace[0] && (strchr(CHANMETA, lastonplace[0]) != NULL))
    sprintf(word1, "on IRC channel %s", lastonplace);
  else if (lastonplace[0] == '@')
    sprintf(word1, "on %s", lastonplace + 1);
  else if (lastonplace[0] != 0)
    sprintf(word1, "on my %s", lastonplace);
  else
    strcpy(word1, "seen");
  dprintf(idx, "%s%s%s was last %s %s\n",
	  prefix, whoredirect, whotarget, word1, word2);
}

static char fixit[512];
static char *fixnick(char *nick)
{
  strcpy(fixit, nick);
  strcat(fixit, "'");
  switch (nick[strlen(nick) - 1]) {
  case 's':
  case 'S':
  case 'x':
  case 'X':
  case 'z':
  case 'Z':
    break;
  default:
    strcat(fixit, "s");
    break;
  }
  return fixit;
}

static char *match_trigger(char *word)
{
  trig_data *t = trigdata;

  while (t->key) {
    if (!strcasecmp(word, t->key))
      return t->text;
    t++;
  }
  return (char *) 0;
}

static char *getxtra(char *hand, char *field)
{

  struct userrec *urec;
  struct user_entry *ue;
  struct xtra_key *xk;

  urec = get_user_by_handle(userlist, hand);
  if (urec) {
    ue = find_user_entry(&USERENTRY_XTRA, urec);
    if (ue)
      for (xk = ue->u.extra; xk; xk = xk->next)
	if (xk->key && !strcasecmp(xk->key, field)) {
	  if (xk->data[0] == '{' && xk->data[strlen(xk->data) - 1] == '}' &&
	      strlen(xk->data) > 2) {
	    strncpy(fixit, &xk->data[1], strlen(xk->data - 9));
	    fixit[strlen(xk->data) - 2] = 0;
	    return fixit;
	  } else {
	    return xk->data;
	  }
	}
  }
  return "";
}

static void wordshift(char *first, char *rest)
{
  char *p, *q = rest;

  LOOPIT:
  p = newsplit(&q);
  strcpy(first, p);
  strcpy(rest, q);
  if (!strcasecmp(first, "and") || !strcasecmp(first, "or"))
    goto LOOPIT;
}

/* Report on current seen info for .modulestat. */
static void seen_report(int idx, int details)
{
  if (details)
    dprintf(idx, "    seen.so - PUB, DCC and MSG \"seen\" commands.\n");
}

/* PUB channel builtin commands. */
static cmd_t seen_pub[] =
{
  {"seen", "", pub_seen, 0},
};

static cmd_t seen_dcc[] =
{
  {"seen", "", dcc_seen, 0},
};

static cmd_t seen_msg[] =
{
  {"seen", "", msg_seen, 0},
};

static int server_seen_setup(char *mod)
{
  p_tcl_bind_list H_temp;

  if ((H_temp = find_bind_table("msg")))
    add_builtins(H_temp, seen_msg, 1);
  return 0;
}

static int irc_seen_setup(char *mod)
{
  p_tcl_bind_list H_temp;

  if ((H_temp = find_bind_table("pub")))
    add_builtins(H_temp, seen_pub, 1);
  return 0;
}

static cmd_t seen_load[] =
{
  {"server", "", server_seen_setup, 0},
  {"irc", "", irc_seen_setup, 0},
};

static char *seen_close()
{
  p_tcl_bind_list H_temp;

  rem_builtins(H_load, seen_load, 2);
  rem_builtins(H_dcc, seen_dcc, 1);
  rem_help_reference("seen.help");
  if ((H_temp = find_bind_table("pub")))
    rem_builtins(H_temp, seen_pub, 1);
  if ((H_temp = find_bind_table("msg")))
    rem_builtins(H_temp, seen_msg, 1);
  module_undepend(MODULE_NAME);
  return NULL;
}

char *seen_start();

static Function seen_table[] =
{
  (Function) seen_start,
  (Function) seen_close,
  (Function) seen_expmem,
  (Function) seen_report,
};

char *seen_start(Function * egg_func_table)
{
  global = egg_func_table;

  context;
  module_register(MODULE_NAME, seen_table, 2, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 103, 0))
    return
      "MODULE `seen' cannot be loaded on Eggdrops prior to version 1.3.0";
  add_builtins(H_load, seen_load, 2);
  add_builtins(H_dcc, seen_dcc, 1);
  add_help_reference("seen.help");
  server_seen_setup(0);
  irc_seen_setup(0);
  trigdata[4].key = botnetnick;
  return NULL;
}
