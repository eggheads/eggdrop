/*
 * language.c -- handles:
 *   language support code
 *
 * $Id: language.c,v 1.31 2011/02/13 14:19:33 simple Exp $
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
int exist_lang_section(char *);
static char *get_specific_langfile(char *, lang_sec *);
static char *get_langfile(lang_sec *);
static int split_lang(char *, char **, char **);
int cmd_loadlanguage(struct userrec *, int, char *);


/* Add a new preferred language to the list of languages. Newly added
 * languages get the highest priority.
 */
void add_lang(char *lang)
{
  lang_pri *lp = langpriority, *lpo = NULL;

  while (lp) {
    /* The language already exists, moving to the beginning */
    if (!strcmp(lang, lp->lang)) {
      /* Already at the front? */
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

  /* No existing entry, create a new one */
  lp = nmalloc(sizeof(lang_pri));
  lp->lang = nmalloc(strlen(lang) + 1);
  strcpy(lp->lang, lang);
  lp->next = NULL;

  /* If we have other entries, point to the beginning of the old list */
  if (langpriority)
    lp->next = langpriority;
  langpriority = lp;
  debug1("LANG: Language loaded: %s", lang);
}

/* Remove a language from the list of preferred languages.
 */
static int del_lang(char *lang)
{
  lang_pri *lp = langpriority, *lpo = NULL;

  while (lp) {
    /* Found the language? */
    if (!strcmp(lang, lp->lang)) {
      if (lpo)
        lpo->next = lp->next;
      else
        langpriority = lp->next;
      if (lp->lang)
        nfree(lp->lang);
      nfree(lp);
      debug1("LANG: Language unloaded: %s", lang);
      return 1;
    }
    lpo = lp;
    lp = lp->next;
  }
  /* Language not found */
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

/* Recheck all sections and check if any language files are available
 * which match the preferred language(s) more closely
 */
static void recheck_lang_sections(void)
{
  lang_sec *ls;
  char *langfile;

  for (ls = langsection; ls && ls->section; ls = ls->next) {
    langfile = get_langfile(ls);
    /* Found a language with a more preferred language? */
    if (langfile) {
      read_lang(langfile);
      nfree(langfile);
    }
  }
}

/* Parse a language file
 */
static void read_lang(char *langfile)
{
  FILE *FLANG;
  char lbuf[512];
  char *ltext = NULL;
  char *ctmp, *ctmp1;
  int lidx;
  int lnew = 1;
  int lline = 1;
  int lskip = 0;
  int ltexts = 0;
  int ladd = 0, lupdate = 0;

  FLANG = fopen(langfile, "r");
  if (FLANG == NULL) {
    putlog(LOG_MISC, "*", "LANG: unexpected: reading from file %s failed.",
           langfile);
    return;
  }

  for (*(ltext = nmalloc(sizeof lbuf)) = 0; fgets(lbuf, sizeof lbuf, FLANG);
       ltext = nrealloc(ltext, strlen(ltext) + sizeof lbuf), lskip = 0) {
    if (lnew) {
      if ((lbuf[0] == '#') || (sscanf(lbuf, "%s", ltext) == EOF))
        lskip = 1;
      else if (sscanf(lbuf, "0x%x,", &lidx) != 1) {
        putlog(LOG_MISC, "*", "LANG: Malformed text line in %s at %d.",
               langfile, lline);
        lskip = 1;
      }
      if (lskip) {
        while (!strchr(lbuf, '\n')) {
          fgets(lbuf, 511, FLANG);
          lline++;
        }
        lline++;
        lnew = 1;
        continue;
      }
      strcpy(ltext, strchr(lbuf, ',') + 1);
    } else
      strcpy(strchr(ltext, 0), lbuf);
    if ((ctmp = strchr(ltext, '\n'))) {
      lline++;
      *ctmp = 0;
      if (ctmp[-1] == '\\') {
        lnew = 0;
        ctmp[-1] = 0;
      } else {
      	ltexts++;
      	lnew = 1;
      	/* Convert literal \n and \t escapes */
      	for (ctmp1 = ctmp = ltext; *ctmp1; ctmp++, ctmp1++) {
          if ((*ctmp1 == '\\') && ctmp1[1] == 'n') {
            *ctmp = '\n';
            ctmp1++;
          } else if ((*ctmp1 == '\\') && ctmp1[1] == 't') {
            *ctmp = '\t';
            ctmp1++;
          } else
            *ctmp = *ctmp1;
        }
        *ctmp = 0;
        if (add_message(lidx, ltext)) {
          lupdate++;
        } else
          ladd++;
      }
    } else
      lnew = 0;
  }
  nfree(ltext);
  fclose(FLANG);

  debug3("LANG: %d messages of %d lines loaded from %s", ltexts, lline,
         langfile);
  debug2("LANG: %d adds, %d updates to message table", ladd, lupdate);
}

/* Returns 1 if the section exists, otherwise 0.
 */
int exist_lang_section(char *section)
{
  lang_sec *ls;

  for (ls = langsection; ls; ls = ls->next)
    if (!strcmp(section, ls->section))
      return 1;
  return 0;
}

/* Add a new language section. e.g. section "core"
 * Load an apropriate language file for the specified section.
 */
void add_lang_section(char *section)
{
  char *langfile = NULL;
  lang_sec *ls, *ols = NULL;
  int ok = 0;

  for (ls = langsection; ls; ols = ls, ls = ls->next)
    /* Already know of that section? */
    if (!strcmp(section, ls->section))
      return;

  /* Create new section entry */
  ls = nmalloc(sizeof(lang_sec));
  ls->section = nmalloc(strlen(section) + 1);
  strcpy(ls->section, section);
  ls->lang = NULL;
  ls->next = NULL;

  /* Connect to existing list of sections */
  if (ols)
    ols->next = ls;
  else
    langsection = ls;
  debug1("LANG: Section loaded: %s", section);

  /* Always load base language */
  langfile = get_specific_langfile(BASELANG, ls);
  if (langfile) {
    read_lang(langfile);
    nfree(langfile);
    ok = 1;
  }
  /* Now overwrite base language with a more preferred one */
  langfile = get_langfile(ls);
  if (!langfile) {
    if (!ok)
      putlog(LOG_MISC, "*", "LANG: No lang files found for section %s.",
             section);
    return;
  }
  read_lang(langfile);
  nfree(langfile);
}

int del_lang_section(char *section)
{
  lang_sec *ls, *ols;

  for (ls = langsection, ols = NULL; ls; ols = ls, ls = ls->next)
    if (ls->section && !strcmp(ls->section, section)) {
      if (ols)
        ols->next = ls->next;
      else
        langsection = ls->next;
      nfree(ls->section);
      if (ls->lang)
        nfree(ls->lang);
      nfree(ls);
      debug1("LANG: Section unloaded: %s", section);
      return 1;
    }
  return 0;
}

static char *get_specific_langfile(char *language, lang_sec *sec)
{
  char *ldir = getenv("EGG_LANGDIR");
  char *langfile;

  if (!ldir)
    ldir = LANGDIR;
  langfile = nmalloc(strlen(ldir) + strlen(sec->section) + strlen(language) +
             8);
  sprintf(langfile, "%s/%s.%s.lang", ldir, sec->section, language);

  if (file_readable(langfile)) {
    /* Save language used for this section */
    sec->lang = nrealloc(sec->lang, strlen(language) + 1);
    strcpy(sec->lang, language);
    return langfile;
  }

  nfree(langfile);
  return NULL;
}

/* Searches for available language files and returns the file with the
 * most preferred language.
 */
static char *get_langfile(lang_sec *sec)
{
  char *langfile;
  lang_pri *lp;

  for (lp = langpriority; lp; lp = lp->next) {
    /* There is no need to reload the same language */
    if (sec->lang && !strcmp(sec->lang, lp->lang))
      return NULL;
    langfile = get_specific_langfile(lp->lang, sec);
    if (langfile)
      return langfile;
  }
  /* We did not find any files, clear the language field */
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

/* Compability function to allow users/modules to use the old command.
 */
int cmd_loadlanguage(struct userrec *u, int idx, char *par)
{
  char *section, *lang, *buf;

  dprintf(idx, "Note: This command is obsoleted by +lang.\n");
  if (!par || !par[0]) {
    dprintf(idx, "Usage: language <section>.<language>\n");
    return 0;
  }
  if (idx != DP_LOG)
    putlog(LOG_CMDS, "*", "#%s# language %s", dcc[idx].nick, par);
  buf = nmalloc(strlen(par) + 1);
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
  lang_tab *l;

  if (!idx)
    return "MSG-0-";
  for (l = langtab[idx & 63]; l; l = l->next)
    if (idx == l->idx)
      return l->text;
  egg_snprintf(text, sizeof text, "MSG%03X", idx);
  return text;
}

int expmem_language()
{
  lang_tab *l;
  lang_sec *ls;
  lang_pri *lp;
  int i, size = 0;

  for (i = 0; i < 64; i++)
    for (l = langtab[i]; l; l = l->next) {
      size += sizeof(lang_tab);
      size += (strlen(l->text) + 1);
    }
  for (ls = langsection; ls; ls = ls->next) {
    size += sizeof(lang_sec);
    if (ls->section)
      size += strlen(ls->section) + 1;
    if (ls->lang)
      size += strlen(ls->lang) + 1;
  }
  for (lp = langpriority; lp; lp = lp->next) {
    size += sizeof(lang_pri);
    if (lp->lang)
      size += strlen(lp->lang) + 1;
  }
  return size;
}

/* A report on the module status - only for debugging purposes
 */
static int cmd_languagestatus(struct userrec *u, int idx, char *par)
{
  int ltexts = 0;
  register int i, c, maxdepth = 0, used = 0, empty = 0;
  lang_tab *l;
  lang_sec *ls = langsection;
  lang_pri *lp = langpriority;

  putlog(LOG_CMDS, "*", "#%s# lstat %s", dcc[idx].nick, par);
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
  dprintf(idx, "Language code report:\n");
  dprintf(idx, "   Table size   : %d bytes\n", expmem_language());
  dprintf(idx, "   Text messages: %d\n", ltexts);
  dprintf(idx, "   %d used, %d unused, maxdepth %d, avg %f\n",
          used, empty, maxdepth, (float) ltexts / 64.0);
  if (lp) {
    int c = 0;

    dprintf(idx, "   Supported languages:");
    for (; lp; lp = lp->next) {
      dprintf(idx, "%s %s", c ? "," : "", lp->lang);
      c = 1;
    }
    dprintf(idx, "\n");
  }
  if (ls) {
    dprintf(idx, "\n   SECTION              LANG\n");
    dprintf(idx, "   ==============================\n");
    for (; ls; ls = ls->next)
      dprintf(idx, "   %-20s %s\n", ls->section,
              ls->lang ? ls->lang : "<none>");
  }
  return 0;
}

/* Compability function to allow scripts to use the old command.
 */
static int tcl_language STDVAR
{
  char *lang, *section, *buf;

  putlog(LOG_MISC, "*", "The Tcl command 'language' is obsolete. Use "
         "'addlang' instead.");
  BADARGS(2, 2, " language");

  buf = nmalloc(strlen(argv[1]) + 1);
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

static cmd_t langdcc[] = {
  {"language", "n",  cmd_loadlanguage,   NULL},
  {"+lang",    "n",  cmd_plslang,        NULL},
  {"-lang",    "n",  cmd_mnslang,        NULL},
  {"+lsec",    "n",  cmd_plslsec,        NULL},
  {"-lsec",    "n",  cmd_mnslsec,        NULL},
  {"ldump",    "n",  cmd_languagedump,   NULL},
  {"lstat",    "n",  cmd_languagestatus, NULL},
  {"relang",   "n",  cmd_relang,         NULL},
  {NULL,       NULL, NULL,               NULL}
};

static tcl_cmds langtcls[] = {
  {"language",             tcl_language},
  {"addlang",               tcl_plslang},
  {"dellang",               tcl_mnslang},
  {"addlangsection", tcl_addlangsection},
  {"dellangsection", tcl_dellangsection},
  {"relang",                 tcl_relang},
  {NULL,                           NULL}
};

void init_language(int flag)
{
  int i;
  char *deflang;

  if (flag) {
    for (i = 0; i < 32; i++)
      langtab[i] = 0;
    /* The default language is always BASELANG as language files are
     * gauranteed to exist in that language.
     */
    add_lang(BASELANG);
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
