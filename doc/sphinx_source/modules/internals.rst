Eggdrop Bind Internals
======================

This document is intended for C developers who want to understand how Eggdrop’s Tcl binds or C binds work.

For documentation purposes the “dcc” bind type is used as an example.

It already exists and is suitable to illustrate the details of bind handling in Eggdrop.

Note: All code snippets are altered for brevity and simplicity, see original source code for the full and current versions.

Bind Table Creation
-------------------

The bind table is added by calling, either at module initialization or startup::

  /* Global symbol, available to other C files with
   * extern p_tcl_bind_list H_dcc;
   */
  p_tcl_bind_list H_dcc;

  /* Creating the bind table:
   * @param[in] const char *name Limited in length, see tclhash.h
   * @param[in] int flags        HT_STACKABLE or 0
   * @param[in] IntFunc          Function pointer to C handler
   * @return    p_tcl_bind_list  aka (tcl_bind_list_t *)
   */
  H_dcc = add_bind_table("dcc", 0, builtin_dcc);

What the :code:`C handler` does is explained later, because a lot happens before it is actually called. :code:`IntFunc` is a generic function pointer that returns an :code:`int` with arbitrary arguments.

:code:`H_dcc` can be exported from core and imported into modules as any other variable or function. That should be explained in a separate document.

Stackable Binds: HT_STACKABLE
-----------------------------

:code:`HT_STACKABLE` means that multiple binds can exist for the same mask.
::

  bind dcc - test proc1; # not stackable
  bind dcc - test proc2; # overwrites the first one, only proc2 will be called

It does not automatically call multiple binds that match, see later in the `Triggering any Bind`_ section for details.

Tcl Binding
-----------

After the bind table is created with :code:`add_bind_table`, Tcl procs can already be registered to this bind by calling::

  bind dcc -|- test myproc
  proc myproc {args} {
    putlog "myproc was called, argument list: '[join $args ',']'"
    return 0
  }

Of course it is not clear so far:

* If flags :code:`-|-` matter for this bind at all and what they are checked against
* If channel flags have a meaning or global/bot only
* What :code:`test` is matched against to see if the bind should trigger
* Which arguments :code:`myproc` receives, the example just accepts all arguments

Triggering the Bind
-------------------

To trigger the bind and call it with the desired arguments, a function is created.
::

  int check_tcl_dcc(const char *cmd, int idx, const char *args) {
    struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
    int x;
    char s[11];

    get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
    egg_snprintf(s, sizeof s, "%ld", dcc[idx].sock);
    Tcl_SetVar(interp, "_dcc1", (char *) dcc[idx].nick, 0);
    Tcl_SetVar(interp, "_dcc2", (char *) s, 0);
    Tcl_SetVar(interp, "_dcc3", (char *) args, 0);
    x = check_tcl_bind(H_dcc, cmd, &fr, " $_dcc1 $_dcc2 $_dcc3",
                    MATCH_PARTIAL | BIND_USE_ATTR | BIND_HAS_BUILTINS);
    /* snip ..., return code handling */
    return 0;
  }

The global Tcl variables :code:`$_dcc1 $_dcc2 $_dcc3` are used as temporary string variables and passed as arguments to the registered Tcl proc.

This shows which arguments the callbacks in Tcl get:

* the nickname of the DCC chat user (handle of the user)
* the IDX (socket id) of the partyline so :code:`[putdcc]` can respond back
* another string argument that depends on the caller

The call to :code:`check_tcl_dcc` can be found in the DCC parsing in `src/dcc.c`.

Triggering any Bind
-------------------

`check_tcl_bind` is used by all binds and does the following::

  /* Generic function to call one/all matching binds
   * @param[in] tcl_bind_list_t *tl      Bind table (e.g. H_dcc)
   * @param[in] const char *match        String to match the bind-masks against
   * @param[in] struct flag_record *atr  Flags of the user calling the bind
   * @param[in] const char *param        Arguments to add to the bind callback proc (e.g. " $_dcc1 $_dcc2 $_dcc3")
   * @param[in] int match_type           Matchtype and various flags
   * @returns   int                      Match result code
   */

  /* Source code changed, only illustrative */
  int check_tcl_bind(tcl_bind_list_t *tl, const char *match, struct flag_record *atr, const char *param, int match_type) {
    int x = BIND_NOMATCH;
    for (tm = tl->first; tm && !finish; tm_last = tm, tm = tm->next) {
      /* Check if bind mask matches */
      if (!check_bind_match(match, tm->mask, match_type))
        continue;
      for (tc = tm->first; tc; tc = tc->next) {
        /* Check if the provided flags suffice for this command. */
        if (check_bind_flags(&tc->flags, atr, match_type)) {
          tc->hits++;
          /* not much more than Tcl_Eval(interp, "<procname> <arguments>"); and grab the result */
          x = trigger_bind(tc->func_name, param, tm->mask);
        }
      }
    }
    return x;
  }

The supplied flags to :code:`check_tcl_bind` in `check_tcl_dcc` are what defines how matching is performed.

In the case of a DCC bind we had:

* Matchtype :code:`MATCH_PARTIAL`: Prefix-Matching if the command can be uniquely identified (e.g. dcc .help calls .help)
* Additional flag :code:`BIND_USE_ATTR`: Flags are checked
* Additional flag :code:`BIND_HAS_BUILTINS`: Something with flag matching, unsure

For details on the available match types (wildcard matching, exact matching, etc.) see :code:`src/tclegg.h`. Additional flags are also described there as well as the return codes of :code:`check_tcl_bind` (e.g. :code:`BIND_NOMATCH`).

Note: For a bind type to be stackable it needs to be registered with :code:`HT_STACKABLE` AND :code:`check_tcl_bind` must be called with :code:`BIND_STACKABLE`.

C Binding
---------

To create a C function that is called by the bind, Eggdrop provides the :code:`add_builtins` function.
::

  /* Add a list of C function callbacks to a bind
   * @param[in] tcl_bind_list_t *  the bind type (e.g. H_dcc)
   * @param[in] cmd_t *            a NULL-terminated table of binds:
   * cmd_t *mycmds = {
   *   {char *name, char *flags, IntFunc function, char *tcl_name},
   *   ...,
   *   {NULL, NULL, NULL, NULL}
   * };
   */
  void add_builtins(tcl_bind_list_t *tl, cmd_t *cc) {
    char p[1024];
    cd_tcl_cmd tclcmd;

    tclcmd.name = p;
    tclcmd.callback = tl->func;
    for (i = 0; cc[i].name; i++) {
      /* Create Tcl command with automatic or given names *<bindtype>:<funcname>, e.g.
       * - H_raw {"324", "", got324, "irc:324"} => *raw:irc:324
       * - H_dcc {"boot", "t", cmd_boot, NULL} => *dcc:boot
       */
      egg_snprintf(p, sizeof p, "*%s:%s", tl->name, cc[i].funcname ? cc[i].funcname : cc[i].name);
      /* arbitrary void * can be included, we include C function pointer */
      tclcmd.cdata = (void *) cc[i].func;
      add_cd_tcl_cmd(tclcmd);
      bind_bind_entry(tl, cc[i].flags, cc[i].name, p);
    }
  }

It automatically creates Tcl commands (e.g. :code:`*dcc:cmd_boot`) that will call the `C handler` from `add_bind_table` in the first section `Bind Table Creation`_ and it gets a context (void \*) argument with the C function it is supposed to call (e.g. `cmd_boot()`).

Now we can actually look at the C function handler for dcc as an example and what it has to implement.

C Handler
---------

The example handler for DCC looks as follows::

  /* Typical Tcl_Command arguments, just like e.g. tcl_putdcc is a Tcl/C command for [putdcc] */
  static int builtin_dcc (ClientData cd, Tcl_Interp *irp, int argc, char *argv[]) {
    int idx;
    /* F: The C function we want to call, if the bind is okay, e.g. cmd_boot() */
    Function F = (Function) cd;

    /* Task of C function: verify argument count and syntax as any Tcl command */
    BADARGS(4, 4, " hand idx param");

    /* C Macro only used in C handlers for bind types, sanity checks the Tcl proc name
     * for *<bindtype>:<name> and that we are in the right C handler
     */
    CHECKVALIDITY(builtin_dcc);

    idx = findidx(atoi(argv[2]));
    if (idx < 0) {
        Tcl_AppendResult(irp, "invalid idx", NULL);
        return TCL_ERROR;
    }

    /* Call the desired C function, e.g. cmd_boot() with their arguments */
    F(dcc[idx].user, idx, argv[3]);
    Tcl_ResetResult(irp);
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }

This is finally the part where we see the arguments a C function gets for a DCC bind as opposed to a Tcl proc.

code:`F(dcc[idx].user, idx, argv[3])`:

* User information as struct userrec *
* IDX as int
* The 3rd string argument from the Tcl call to \*dcc:cmd_boot, which was :code:`$_dcc3` which was :code:`args` to :code:`check_tcl_dcc` which was everything after the dcc command

So this is how we register C callbacks for binds with the correct arguments::

  /* We know the return value is ignored because the return value of F
   * in builtin_dcc is ignored, so it can be void, but for other binds
   * it could be something else and used in the C handler for the bind.
   */
  void cmd_boot(struct userrec *u, int idx, char *par) { /* snip */ }

  cmd_t *mycmds = {
    {"boot", "t", (IntFunc) cmd_boot, NULL /* automatic name: *dcc:boot */},
    {NULL, NULL, NULL, NULL}
  };
  add_builtins(H_dcc, mycmds);

Summary
-------

In summary, this is how the dcc bind is called:

* :code:`check_tcl_dcc()` creates Tcl variables :code:`$_dcc1 $_dcc2 $_dcc3` and lets :code:`check_tcl_bind` call the binds
* Tcl binds are done at this point
* C binds mean the Tcl command associated with the bind is :code:`*dcc:boot` which calls :code:`builtin_dcc` which gets :code:`cmd_boot` as ClientData cd argument
* :code:`gbuildin_dcc` performs some sanity checking to avoid crashes and then calls :code:`cmd_boot()` aka :code:`F()` with the arguments it wants C callbacks to have

Example edited and annotated gdb backtrace in :code::`cmd_boot` after doing :code:`.boot test` on the partyline as user :code:`thommey` with typical owner flags.
::

  #0  cmd_boot (u=0x55e8bd8a49b0, idx=4, par=0x55e8be6a0010 "test") at cmds.c:614
      *u = {next = 0x55e8bd8aec90, handle = "thommey", flags = 8977024, flags_udef = 0, chanrec = 0x55e8bd8aeae0, entries = 0x55e8bd8a4a10}
  #1  builtin_dcc (cd=0x55e8bbf002d0 <cmd_boot>, irp=0x55e8bd59b1c0, argc=4, argv=0x55e8bd7e3e00) at tclhash.c:678
      idx = 4
      argv = {0x55e8be642fa0 "*dcc:boot", 0x55e8be9f6bd0 "thommey", 0x55e8be7d9020 "4", 0x55e8be6a0010 "test", 0x0}
      F = 0x55e8bbf002d0 <cmd_boot>
  #5  Tcl_Eval (interp=0x55e8bd59b1c0, script = "*dcc:boot $_dcc1 $_dcc2 $_dcc3") from /usr/lib/x86_64-linux-gnu/libtcl8.6.so
      Tcl: return $_dcc1 = "thommey"
      Tcl: return $_dcc2 = "4"
      Tcl: return $_dcc3 = "test"
      Tcl: return $lastbind = "boot" (set automatically by trigger_bind)
  #8  trigger_bind (proc=proc@entry=0x55e8bd5efda0 "*dcc:boot", param=param@entry=0x55e8bbf4112b " $_dcc1 $_dcc2 $_dcc3", mask=mask@entry=0x55e8bd5efd40 "boot") at tclhash.c:742
  #9  check_tcl_bind (tl=0x55e8bd5eecb0 <H_dcc>, match=match@entry=0x7ffcf3f9dac1 "boot", atr=atr@entry=0x7ffcf3f9d100, param=param@entry=0x55e8bbf4112b " $_dcc1 $_dcc2 $_dcc3", match_type=match_type@entry=80) at tclhash.c:942
      proc = 0x55e8bd5efda0 "*dcc:boot"
      mask = 0x55e8bd5efd40 "boot"
      brkt = 0x7ffcf3f9dac6 "test"
  #10 check_tcl_dcc (cmd=cmd@entry=0x7ffcf3f9dac1 "boot", idx=idx@entry=4, args=0x7ffcf3f9dac6 "test") at tclhash.c:974
      fr = {match = 5, global = 8977024, udef_global = 0, bot = 0, chan = 0, udef_chan = 0}
  #11 dcc_chat (idx=idx@entry=4, buf=<optimized out>, i=<optimized out>) at dcc.c:1068
      v = 0x7ffcf3f9dac1 "boot"
