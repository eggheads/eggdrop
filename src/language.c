/*
 * language.c - language support code.
 */

/*
 * DOES:
 *              Nothing <- typical BB code :)
 *
 * WILL DO:
 *              Upon loading:
 *              o       default loads core.english.lang, if possible.
 *              Commands:
 *              DCC .language <language>
 *              DCC .ldump
 *
 * FILE FORMAT: language.lang
 *              <textidx>,<text>
 * TEXT MESSAGE USAGE:
 *              get_language(<textidx> [,<PARMS>])
 *
 * ADDING LANGUAGES:
 *              o       Copy an existing language.lang to a new .lang
 *                      file and modify as needed.  Use %1...%n, where
 *                      necessary, for plug-in insertions of parameters
 *                      (see language.lang).
 *              o       Ensure <language>.lang is in the lang directory.
 *              o       .language <language>
 *
 */

#include "main.h"

extern struct dcc_t *dcc;

static int langloaded = 0;

typedef struct lang_t {
  int idx;
  char *text;
  struct lang_t *next;
} lang_tab;

static lang_tab *langtab[64];

static int add_message(int lidx, char *ltext)
{
  lang_tab *l = langtab[lidx & 63];

  while (l) {
    if (l->idx && (l->idx == lidx)) {
      nfree(l->text);
      l->text = nmalloc(strlen(ltext) + 1);
      strcpy(l->text, ltext);
      return 1;
    }
    if (!l->next)
      break;
    l = l->next;
  }
  if (l) {
    l->next = nmalloc(sizeof(lang_tab));
    l = l->next;
  } else
    l = langtab[lidx & 63] = nmalloc(sizeof(lang_tab));
  l->idx = lidx;
  l->text = nmalloc(strlen(ltext) + 1);
  strcpy(l->text, ltext);
  l->next = 0;
  return 0;
}

int cmd_loadlanguage(struct userrec *u, int idx, char *par)
{
  FILE *FLANG;
  char lbuf[512];
  char *langfile = NULL;
  char *ltext = NULL;
  char *ctmp, *ctmp1;
  char *ldir;
  int lidx;
  int lline = 0;
  int lskip;
  int ltexts = 0;
  int ladd = 0, lupdate = 0;

  context;
  langloaded = 0;
  if (!par || !par[0]) {
    dprintf(idx, "Usage: language <language>\n");
    return 0;
  }
  if (idx != DP_LOG)
    putlog(LOG_CMDS, "*", "#%s# language %s", dcc[idx].nick, par);
  if (par[0] == '.' || par[0] == '/') {
    langfile = nmalloc(strlen(par) + 1);
    strcpy(langfile, par);
  } else {
    ldir = getenv("EGG_LANGDIR");
    if (ldir) {
      langfile = nmalloc(strlen(ldir) + strlen(par) + 7);
      sprintf(langfile, "%s/%s.lang", ldir, par);
    } else {
      langfile = nmalloc(strlen(LANGDIR) + strlen(par) + 7);
      sprintf(langfile, "%s/%s.lang", LANGDIR, par);
    }
  }

  FLANG = fopen(langfile, "r");
  if (FLANG == NULL) {
    dprintf(idx, "Can't load language module: %s\n", langfile);
    nfree(langfile);
    return 0;
  }
  lskip = 0;
  while (fgets(lbuf, 511, FLANG)) {
    lline++;
    if (lbuf[0] != '#' || lskip) {
      ltext = nrealloc(ltext, 512);
      if (sscanf(lbuf, "%s", ltext) != EOF) {
	if (sscanf(lbuf, "0x%x,%500c", &lidx, ltext) != 2) {
	  putlog(LOG_MISC, "*", "Malformed text line in %s at %d.",
		 langfile, lline);
	} else {
	  ltexts++;
	  ctmp = strchr(ltext, '\n');
	  *ctmp = 0;
	  while (ltext[strlen(ltext) - 1] == '\\') {
	    ltext[strlen(ltext) - 1] = 0;
	    if (fgets(lbuf, 511, FLANG)) {
	      lline++;
	      ctmp = strchr(lbuf, '\n');
	      *ctmp = 0;
	      ltext = nrealloc(ltext, strlen(lbuf) + strlen(ltext) + 1);
	      strcpy(strchr(ltext, 0), lbuf);
	    }
	  }
	}
	/* We gotta fix \n's here as, being arguments to sprintf(), */
	/* they won't get translated */
	ctmp = ltext;
	ctmp1 = ltext;
	while (*ctmp1) {
	  if (*ctmp1 == '\\' && *(ctmp1 + 1) == 'n') {
	    *ctmp = '\n';
	    ctmp1++;
	  } else
	    *ctmp = *ctmp1;
	  ctmp++;
	  ctmp1++;
	}
	*ctmp = '\0';
	if (add_message(lidx, ltext)) {
	  lupdate++;
	} else
	  ladd++;
      }
    } else {
      ctmp = strchr(lbuf, '\n');
      if (lskip && (strlen(lbuf) == 1 || *(ctmp - 1) != '\\'))
	lskip = 0;
    }
  }
  nfree(ltext);
  fclose(FLANG);

  putlog(LOG_MISC, "*", "LANG: %d messages of %d lines loaded from %s",
	 ltexts, lline, langfile);
  putlog(LOG_MISC, "*", "LANG: %d adds, %d updates to message table",
	 ladd, lupdate);
  nfree(langfile);
  langloaded = 1;
  return 0;
}

static int cmd_languagedump(struct userrec *u, int idx, char *par)
{
  lang_tab *l;
  char ltext2[512];
  int idx2, i;

  context;
  putlog(LOG_CMDS, "*", "#%s# ldump %s", dcc[idx].nick, par);
  if (par[0]) {
    /* atoi (hence strtol) don't work right here for hex */
    if (strlen(par) > 2 && par[0] == '0' && par[1] == 'x')
      sscanf(par, "%x", &idx2);
    else
      idx2 = (int) strtol(par, (char **) NULL, 10);
    strcpy(ltext2, get_language(idx2));
    dprintf(idx, "0x%x: %s\n", idx2, ltext2);
    return 0;
  }
  dprintf(idx, " LANGIDX TEXT\n");
  for (i = 0; i < 64; i++)
    for (l = langtab[i]; l; l = l->next)
      dprintf(idx, "0x%x   %s\n", l->idx, l->text);
  return 0;
}

static char text[512];
char *get_language(int idx)
{
  lang_tab *l = langtab[idx & 63];

  if (!idx)
    return "MSG-0-";
  while (l) {
    if (idx == l->idx)
      return l->text;
    l = l->next;
  }
  sprintf(text, "MSG%03X", idx);
  return text;
}

int expmem_language()
{
  lang_tab *l;
  int i, size = 0;

  for (i = 0; i < 64; i++)
    for (l = langtab[i]; l; l = l->next) {
      size += sizeof(lang_tab);
      size += (strlen(l->text) + 1);
    }
  return size;
}

/* a report on the module status - not sure if we need this now :/ */
static int cmd_languagestatus(struct userrec *u, int idx, char *par)
{
  int ltexts = 0;
  int maxdepth = 0, used = 0, empty = 0, i, c;
  lang_tab *l;

  for (i = 0; i < 64; i++) {
    c = 0;
    for (l = langtab[i]; l; l = l->next)
      c++;
    if (c > maxdepth)
      maxdepth = c;
    if (c)
      used++;
    else
      empty++;
    ltexts += c;
  }
  context;
  dprintf(idx, "language code report:\n");
  dprintf(idx, "   Table size: %d bytes\n", expmem_language());
  dprintf(idx, "   Text messages: %d\n", ltexts);
  dprintf(idx, "   %d used, %d unused, maxdepth %d, avg %f\n",
	  used, empty, maxdepth, (float) ltexts / 64.0);
  return 0;
}

static int tcl_language STDVAR
{
  BADARGS(2, 2, " language");
  (void) cmd_loadlanguage(0, DP_LOG, argv[1]);
  if (!langloaded) {
    Tcl_AppendResult(irp, "Load failed.", NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static cmd_t langdcc[] =
{
  {"language", "n", cmd_loadlanguage, NULL},
  {"ldump", "n", cmd_languagedump, NULL},
  {"lstat", "n", cmd_languagestatus, NULL},
};

static tcl_cmds langtcls[] =
{
  {"language", tcl_language},
  {0, 0}
};

void init_language(char *default_lang)
{
  int i;

  context;
  if (default_lang) {
    for (i = 0; i < 32; i++)
      langtab[i] = 0;
    cmd_loadlanguage(0, DP_LOG, default_lang);	/* and robey said super-dprintf
						 * was silly :) */
  } else {
    add_tcl_commands(langtcls);
    add_builtins(H_dcc, langdcc, 3);
  }
}
