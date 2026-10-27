/*
 * language.c -- handles:
 *   language support code
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2025 Eggheads Development Team
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
 *              get_language(<textidx> [,<PARAMS>])
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

// ECS like datastruct for fast cache friendly vectorizable binary search
struct entry {
    uint32_t id;     // msg id
    uint32_t off;    // offset into msgs.msg
};
struct lang_msgs {
  struct entry *e;   // array of entries sorted by e.id
  char* msg;         // array of msgs
  uint32_t e_used;
  uint32_t e_size;
  uint32_t msg_used;
  uint32_t msg_size;
} msgs;

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
char language[64];


/* Add a new preferred language to the list of languages. Newly added
 * languages get the highest priority.
 */
static void add_lang(char *lang)
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
  debug1("LANG: Language added to list: %s", lang);
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

static int bsearch_compare(const void *a, const void *b)
{
  const uint32_t *id = a;
  const struct entry *e = b;

  return (*id - e->id);
}

static int add_message(int lidx, char *ltext)
{
  struct entry *e = bsearch(&lidx, msgs.e, msgs.e_used, sizeof(msgs.e[0]), bsearch_compare);
  int old_size, add_size, i;
  size_t ltext_size = strlen(ltext) + 1;

  if (e) {
    old_size = strlen(msgs.msg + e->off) + 1;
    if (ltext_size != old_size) {
      add_size = ltext_size - old_size;

      // add_size can be posive or negative
      // the following code handles both cases equally
      if ((msgs.msg_used + add_size) > msgs.msg_size) {
        // resize msgs.msg
        msgs.msg_size += MAX(1024, add_size);
        msgs.msg = nrealloc(msgs.msg, msgs.msg_size);
      }
      memmove(msgs.msg + e->off + ltext_size,
              msgs.msg + e->off + old_size,
              msgs.msg_used - e->off - old_size);
      msgs.msg_used += add_size;
      // update offsets
      for (i = 0; i < msgs.e_used; i++)
        if (msgs.e[i].off > e->off)
          msgs.e[i].off += add_size;

    }
    memcpy(msgs.msg + e->off, ltext, ltext_size);
    return 1;
  }
  if (msgs.e_used == msgs.e_size) {
    // resize e
    msgs.e_size += 1024;
    msgs.e = nrealloc(msgs.e, msgs.e_size * sizeof(msgs.e[0]));
  }
  e = &msgs.e[msgs.e_used];
  e->id = lidx;
  e->off = msgs.msg_used;
  if ((msgs.msg_used += ltext_size) > msgs.msg_size) {
    // resize msgs.msg
    msgs.msg_size += MAX(1024, ltext_size);
    msgs.msg = nrealloc(msgs.msg, msgs.msg_size);
  }
  memcpy(msgs.msg + e->off, ltext, ltext_size);
  msgs.e_used++;
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
static int qsort_compare(const void *p1, const void *p2)
{
  const struct entry *left = (const struct entry *)p1;
  const struct entry *right = (const struct entry *)p2;

  return ((left->id > right->id) - (left->id < right->id));
}

/* Parse a language file
 */
static void read_lang(char *langfile)
{
  FILE *FLANG;
  char lbuf[256];
  char *ltext = NULL;
  char *ctmp, *ctmp1;
  int ltextsize = sizeof lbuf;
  unsigned int lidx;
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
       lskip = 0) {
    if (lnew) {
      if ((lbuf[0] == '#') || (sscanf(lbuf, "%s", ltext) == EOF))
        lskip = 1;
      else if (sscanf(lbuf, "0x%x,", &lidx) != 1) {
        putlog(LOG_MISC, "*", "LANG: Malformed text line in %s at %d.",
               langfile, lline);
        lskip = 1;
      }
      if (lskip) {
        while (!strchr(lbuf, '\n') && fgets(lbuf, sizeof lbuf, FLANG) != NULL) {
          lline++;
        }
        /* fgets == NULL means error or empty file, so check for error */
        if (ferror(FLANG)) {
          putlog(LOG_MISC, "*", "LANG: Error reading lang file.");
        }
        lnew = 1;
        continue;
      }
      if ((ctmp = strchr(lbuf, ',')))
        strcpy(ltext, ctmp + 1);
      else 
        putlog(LOG_MISC, "*", "LANG: Malformed text line (missing ,) in %s at %d.",
               langfile, lline);
    } else {
      int len = strlen(ltext);
      if ((len + strlen(lbuf) + 1) > ltextsize) {
        ltextsize += sizeof lbuf;
        ltext = nrealloc(ltext, ltextsize);
      }
      strcpy(ltext + len, lbuf);
    }
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
  // shrink message index and message buffer
  msgs.e = nrealloc(msgs.e, msgs.e_used * sizeof(msgs.e[0]));
  msgs.e_size = msgs.e_used;
  msgs.msg = nrealloc(msgs.msg, msgs.msg_used);
  msgs.msg_size = msgs.msg_used;
  // sort message entries by message id
  qsort(msgs.e, msgs.e_used, sizeof(msgs.e[0]), qsort_compare);
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
 * Load an appropriate language file for the specified section.
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
 * Only needed for compatibility functions.
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

/* Compatibility function to allow users/modules to use the old command.
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
  char ltext2[512];
  unsigned int idx2;
  int i;

  putlog(LOG_CMDS, "*", "#%s# ldump %s", dcc[idx].nick, par);
  if (par[0]) {
    /* atoi (hence strtol) don't work right here for hex */
    if (strlen(par) > 2 && par[0] == '0' && par[1] == 'x')
      sscanf(par, "%x", &idx2);
    else
      idx2 = (int) strtol(par, (char **) NULL, 10);
    strlcpy(ltext2, get_language(idx2), sizeof ltext2);
    dprintf(idx, "0x%x: %s\n", idx2, ltext2);
    return 0;
  }
  dprintf(idx, " LANGIDX TEXT\n");
  for (i = 0; i < msgs.e_used; i++)
    dprintf(idx, "0x%x   %s\n", msgs.e[i].id, msgs.msg + msgs.e[i].off);
  return 0;
}

char *get_language(int idx)
{
  struct entry *e;
  static char text[512];

  if (!idx)
    return "MSG-0-";
  if ((e = bsearch(&idx, msgs.e, msgs.e_used, sizeof(msgs.e[0]), bsearch_compare)))
    return msgs.msg + e->off;
  egg_snprintf(text, sizeof text, "MSG%03X", idx);
  return text;
}

int expmem_language()
{
  lang_sec *ls;
  lang_pri *lp;
  int size = sizeof msgs + msgs.e_size * sizeof(msgs.e[0]) + msgs.msg_size;

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
  lang_sec *ls = langsection;
  lang_pri *lp = langpriority;

  putlog(LOG_CMDS, "*", "#%s# lstat %s", dcc[idx].nick, par);
  dprintf(idx, "Language code report:\n");
  dprintf(idx, "   Table size   : %d bytes\n", expmem_language());
  dprintf(idx, "   Text messages: %u\n", msgs.e_used);
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

/* Compatibility function to allow scripts to use the old command.
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
    Tcl_SetResult(irp, "Invalid parameter", TCL_STATIC);
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

  strlcpy(language, argv[1], sizeof language);
  add_lang(argv[1]);
  recheck_lang_sections();

  return TCL_OK;
}

static int tcl_mnslang STDVAR
{
  BADARGS(2, 2, " language");

  if (!del_lang(argv[1])) {
    Tcl_SetResult(irp, "Language not found.", TCL_STATIC);
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
    Tcl_SetResult(irp, "Section not found", TCL_STATIC);
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
  char *deflang;

  if (flag) {
    // init message index and message buffer
    msgs.e_size = 1024;
    msgs.e = nmalloc(msgs.e_size * sizeof(msgs.e[0]));
    msgs.msg_size = 1024;
    msgs.msg = nmalloc(msgs.msg_size);
    /* The default language is always BASELANG as language files are
     * guaranteed to exist in that language.
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
