/*
 * language.c - language support code.
 */

/*
 * DOES:
 *              Nothing <- typical BB code :)
 *
 * ENVIRONMENT VARIABLES:
 *              EGG_LANG       - language to use (default: "english")
 *              EGG_LANGDIR    - directory with all lang files
 *                               (default: "./language")
 * WILL DO:
 *              Upon loading:
 *              o       default loads section core, if possible.
 *              Commands:
 *              DCC .+lang <language>
 *              DCC .-lang <language>
 *              DCC .+lsec <section>
 *              DCC .-lsec <section>
 *              DCC .relang
 *              DCC .ldump
 *              DCC .lstat
 *
 * FILE FORMAT: language.lang
 *              <textidx>,<text>
 * TEXT MESSAGE USAGE:
 *              get_language(<textidx> [,<PARMS>])
 *
 * ADDING LANGUAGES:
 *              o       Copy an existing <section>.<oldlanguage>.lang to a
 *                      new .lang file and modify as needed.
 *                      Use %s or %d where necessary, for plug-in
 *                      insertions of parameters (see core.english.lang).
 *              o       Ensure <section>.<newlanguage>.lang is in the lang
 *                      directory.
 *              o       .+lang <newlanguage>
 * ADDING SECTIONS:
 *              o       Create a <newsection>.english.lang file.
 *              o       Add add_lang_section("<newsection>"); to your module
 *                      startup function.
 *
 */

#include "main.h"

extern struct dcc_t *dcc;

typedef struct lang_st {
  struct lang_st *next;
  char *lang;
  char *section;
} lang_sec;

typedef struct lang_pr {
  struct lang_pr *next;
  char *lang;
} lang_pri;

typedef struct lang_t {
  int idx;
  char *text;
  struct lang_t *next;
} lang_tab;

static lang_tab *langtab[64];
static lang_sec *langsection = NULL;
static lang_pri *langpriority = NULL;

static int del_lang(char *);
static int add_message(int, char *);
static void recheck_lang_sections(void);
static void read_lang(char *);
void add_lang_section(char *);
int del_lang_section(char *);
static char *get_langfile(char *, lang_sec *);
static int split_lang(char *, char **, char **);
int cmd_loadlanguage(struct userrec *u, int idx, char *par);


/* add a new preferred language to the list of languages. Newly added
 * languages get the highest priority.
 */
void add_lang(char *lang)
{
  lang_pri *lp = langpriority, *lpo = NULL;

  context;
  while (lp) {
    /* the language already exists, moving to the beginning */
    if (!strcmp(lang, lp->lang)) {
      /* already at the front? */
      if (!lpo)
	return;
      lpo->next = lp->next;
      lp->next = lpo;
      langpriority = lp;
      return;
    }
    lpo = lp;
    lp = lp->next;
  }

  /* no existing entry, create a new one */
  lp = nmalloc(sizeof(lang_pri));
  lp->lang = nmalloc(strlen(lang) + 1);
  strcpy(lp->lang, lang);
  lp->next = NULL;

  /* if we have other entries, point to the beginning of the old list */
  if (langpriority)
    lp->next = langpriority;
  langpriority = lp;
  putlog(LOG_MISC, "*", "LANG: Now supporting language %s.", lang); 
}

/* remove a language from the list of preferred languages. */
static int del_lang(char *lang)
{
  lang_pri *lp = langpriority, *lpo = NULL;

  context;
  while (lp) {
    /* found the language? */
    if (!strcmp(lang, lp->lang)) {
      if (lpo)
	lpo->next = lp->next;
      else
        langpriority = lp->next;
      if (lp->lang)
        nfree(lp->lang);
      nfree(lp);
      putlog(LOG_MISC, "*", "LANG: Not supporting language %s any longer.",
	     lang); 
      return 1;
    }
    lpo = lp;
    lp = lp->next;
  }
  /* language not found */
  return 0;
}

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

/* recheck all sections and check if any language files are available
 * which match the preferred language(s) more closely
 */
static void recheck_lang_sections(void)
{
  lang_sec *ls = langsection;
  char *langfile;

  context;
  while (ls) {
    if (ls->section) {
      langfile = get_langfile(ls->section, ls);
      /* found a language with a more preferred language? */
      if (langfile) {
        read_lang(langfile);
        nfree(langfile);
      }
    }
    ls = ls->next;
  }
}

/* parse a language file */
static void read_lang(char *langfile)
{
  FILE *FLANG;
  char lbuf[512];
  char *ltext = NULL;
  char *ctmp, *ctmp1;
  int lidx;
  int lline = 0;
  int lskip;
  int ltexts = 0;
  int ladd = 0, lupdate = 0;

  context;
  FLANG = fopen(langfile, "r");
  if (FLANG == NULL) {
    putlog(LOG_MISC, "*", "LANG: unexpected: reading from file %s failed.");
    return;
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
}

/* Add a new language section. e.g. section "core"
 * Load an apropriate language file for the specified section.
 */
void add_lang_section(char *section)
{
  char *langfile = NULL;
  lang_sec *ls = langsection, *ols = NULL;
 
  context;
  while (ls) {
    /* already know of that section? */
    if (!strcmp(section, ls->section))
      return;
    ols = ls;
    ls = ls->next;
  }

  /* create new section entry */
  ls = nmalloc(sizeof(lang_sec));
  ls->section = nmalloc(strlen(section) + 1);
  strcpy(ls->section, section);
  ls->lang = NULL;
  ls->next = NULL;

  /* connect to existing list of sections */
  if (ols)
    ols->next = ls;
  else
    langsection = ls;
  putlog(LOG_MISC, "*", "LANG: Added section %s.", section);
  
  langfile = get_langfile(section, ls);
  if (!langfile) {
    putlog(LOG_MISC, "*", "LANG: No lang files found for section %s.",
	   section);
    return;
  }
  read_lang(langfile);
  nfree(langfile);
}

int del_lang_section(char *section)
{
  lang_sec *ls = langsection, *ols = NULL;

  while (ls) {
    if (ls->section && !strcmp(ls->section, section)) {
      if (ols)
	ols->next = ls->next;
      else
	langsection = ls->next;
      nfree(ls->section);
      if (ls->lang)
	nfree(ls->lang);
      nfree(ls);
      putlog(LOG_MISC, "*", "LANG: Removed section %s.", section);
      return 1;
    }
    ols = ls;
    ls = ls->next;
  }
  return 0;
}

/* Searches for available language files and returns the file with the
 * most preferred language.
 */
static char *get_langfile(char *section, lang_sec *sec)
{
  FILE *sfile = NULL;
  char *ldir = getenv("EGG_LANGDIR");
  char *langfile;
  lang_pri *lp = langpriority;

  context;
  while (lp) {
    /* there is no need to reload the same language */
    if (sec->lang && !strcmp(sec->lang, lp->lang)) {
      return NULL;
    }
    if (ldir) {
      langfile = nmalloc(strlen(ldir)+strlen(section)+strlen(lp->lang)+8);
      sprintf(langfile, "%s/%s.%s.lang", ldir, section, lp->lang);
    } else {
      langfile = nmalloc(strlen(LANGDIR)+strlen(section)+strlen(lp->lang)+8);
      sprintf(langfile, "%s/%s.%s.lang", LANGDIR, section, lp->lang);
    }
    sfile = fopen(langfile, "r");
    if (sfile) {
      fclose(sfile);
      /* save language used for this section */
      sec->lang = nrealloc(sec->lang, strlen(lp->lang) + 1);
      strcpy(sec->lang, lp->lang);
      return langfile;
    }
    nfree(langfile);
    lp = lp->next;
  }
  /* we did not find any files, clear the language field */
  if (sec->lang)
    nfree(sec->lang);
  sec->lang = NULL;
  return NULL;
}

/* Split up a string /path/<section>.<language>.lang into the
 * needed information for the new language system.
 * Only needed for compability functions.
 */
static int split_lang(char *par, char **lang, char **section)
{
  char *p;

  context;
  p = strrchr(par, '/');
  /* path attached? */
  if (p)
    *section = p + 1;
  else
    *section = par;
  p = strchr(*section, '.');
  if (p)
    p[0] = 0;
  else
    return 0;
  *lang = p + 1;
  p = strstr(*lang, ".lang");
  if (p)
    p[0] = 0;
  return 1;
}

/* compability function to allow users/modules to use the old command. */
int cmd_loadlanguage(struct userrec *u, int idx, char *par)
{
  char *section, *lang, *buf;

  context;
  dprintf(idx, "Note: This command is obsoleted by +lang.\n");
  if (!par || !par[0]) {
    dprintf(idx, "Usage: language <section>.<language>\n");
    return 0;
  }
  if (idx != DP_LOG)
    putlog(LOG_CMDS, "*", "#%s# language %s", dcc[idx].nick, par);
  buf = nmalloc(strlen(par)+1);
  strcpy(buf, par);
  if (!split_lang(buf, &lang, &section)) {
    nfree(buf);
    dprintf(idx, "Invalid parameter %s.\n", par);
    return 0;
  }
  add_lang(lang);
  add_lang_section(section);
  nfree(buf);
  recheck_lang_sections();
  return 0;
}

static int cmd_plslang(struct userrec *u, int idx, char *par)
{
  context;
  if (!par || !par[0]) {
    dprintf(idx, "Usage: +lang <language>\n");
    return 0;
  }
  putlog(LOG_CMDS, "*", "#%s# +lang %s", dcc[idx].nick, par);
  add_lang(par);
  recheck_lang_sections();
  return 0;
}

static int cmd_mnslang(struct userrec *u, int idx, char *par)
{
  context;
  if (!par || !par[0]) {
    dprintf(idx, "Usage: -lang <language>\n");
    return 0;
  }
  putlog(LOG_CMDS, "*", "#%s# -lang %s", dcc[idx].nick, par);
  if (!del_lang(par))
    dprintf(idx, "Language %s not found.\n", par);
  else
    recheck_lang_sections();
  return 0;
}

static int cmd_plslsec(struct userrec *u, int idx, char *par)
{
  context;
  if (!par || !par[0]) {
    dprintf(idx, "Usage: +lsec <section>\n");
    return 0;
  }
  putlog(LOG_CMDS, "*", "#%s# +lsec %s", dcc[idx].nick, par);
  add_lang_section(par);
  return 0;
}

static int cmd_mnslsec(struct userrec *u, int idx, char *par)
{
  context;
  if (!par || !par[0]) {
    dprintf(idx, "Usage: -lsec <section>\n");
    return 0;
  }
  putlog(LOG_CMDS, "*", "#%s# -lsec %s", dcc[idx].nick, par);
  if (!del_lang_section(par))
    dprintf(idx, "Section %s not found.\n", par);
  return 0;
}

static int cmd_relang(struct userrec *u, int idx, char *par)
{
  dprintf(idx, "Rechecking language sections...\n");
  recheck_lang_sections();
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
  lang_sec *ls = langsection;
  lang_pri *lp = langpriority;
  int i, size = 0;

  context;
  for (i = 0; i < 64; i++)
    for (l = langtab[i]; l; l = l->next) {
      size += sizeof(lang_tab);
      size += (strlen(l->text) + 1);
    }
  while (ls) {
    size += sizeof(lang_sec);
    if (ls->section)
      size += strlen(ls->section)+1;
    if (ls->lang)
      size += strlen(ls->lang)+1;
    ls = ls->next;
  }
  while (lp) {
    size += sizeof(lang_pri);
    if (lp->lang)
      size += strlen(lp->lang)+1;
    lp = lp->next;
  }
  return size;
}

/* a report on the module status - not sure if we need this now :/ */
static int cmd_languagestatus(struct userrec *u, int idx, char *par)
{
  int ltexts = 0;
  int maxdepth = 0, used = 0, empty = 0, i, c;
  lang_tab *l;
  lang_sec *ls = langsection;
  lang_pri *lp = langpriority;

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
  if (lp) {
    dprintf(idx, "languages:\n");
    while (lp) {
      dprintf(idx, "   %s\n", lp->lang);
      lp = lp->next;
    }
  }
  if (ls) {
    dprintf(idx, "language sections:\n");
    while (ls) {
      dprintf(idx, "   %s - %s\n", ls->section,
	      ls->lang ? ls->lang : "<none>");
      ls = ls->next;
    }
  }
  return 0;
}

/* compability function to allow scripts to use the old command. */
static int tcl_language STDVAR
{
  char *lang, *section, *buf;
  BADARGS(2, 2, " language");

  buf = nmalloc(strlen(argv[1])+1);
  strcpy(buf, argv[1]);
  if (!split_lang(buf, &lang, &section)) {
    Tcl_AppendResult(irp, "Invalid parameter", NULL);
    nfree(buf);
    return TCL_ERROR;
  }
  add_lang(lang);
  add_lang_section(section);
  nfree(buf);
  recheck_lang_sections();
  return TCL_OK;
}

static int tcl_plslang STDVAR
{
  BADARGS(2, 2, " language");
  
  add_lang(argv[1]);
  recheck_lang_sections();

  return TCL_OK;
}

static int tcl_mnslang STDVAR
{ 
  BADARGS(2, 2, " language");

  if (!del_lang(argv[1])) {
    Tcl_AppendResult(irp, "Language not found.", NULL);
    return TCL_ERROR;
  }
  recheck_lang_sections();

  return TCL_OK;
}

static int tcl_addlangsection STDVAR
{ 
  BADARGS(2, 2, " section");

  add_lang_section(argv[1]);
  return TCL_OK;
}

static int tcl_dellangsection STDVAR
{ 
  BADARGS(2, 2, " section");

  if (!del_lang_section(argv[1])) {
    Tcl_AppendResult(irp, "Section not found", NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static int tcl_relang STDVAR
{
  recheck_lang_sections();
  return TCL_OK;
}

static cmd_t langdcc[] =
{
  {"language", "n", cmd_loadlanguage, NULL},
  {"+lang", "n", cmd_plslang, NULL},
  {"-lang", "n", cmd_mnslang, NULL},
  {"+lsec", "n", cmd_plslsec, NULL},
  {"-lsec", "n", cmd_mnslsec, NULL},
  {"ldump", "n", cmd_languagedump, NULL},
  {"lstat", "n", cmd_languagestatus, NULL},
  {"relang", "n", cmd_relang, NULL},
  {0, 0, 0, 0}
};

static tcl_cmds langtcls[] =
{
  {"language", tcl_language},
  {"addlang", tcl_plslang},
  {"dellang", tcl_mnslang},
  {"addlangsection", tcl_addlangsection},
  {"dellangsection", tcl_dellangsection},
  {"relang", tcl_relang},
  {0, 0}
};

void init_language(int flag)
{
  int i;
  char *deflang;

  context;
  if (flag) {
    for (i = 0; i < 32; i++)
      langtab[i] = 0;
    /* The default language is always "english" as language files are
     * gauranteed to exist in english. */
    add_lang("english");
    /* Let the user choose a different, preferred language */
    deflang = getenv("EGG_LANG");
    if (deflang)
      add_lang(deflang);
    add_lang_section("core");
  } else {
    add_tcl_commands(langtcls);
    add_builtins(H_dcc, langdcc);
  }
}
