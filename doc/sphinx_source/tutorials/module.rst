Writing a Basic Eggdrop Module
==============================

An Eggdrop module is a piece of C code that can be loaded (or unloaded) onto the core Eggdrop code. A module differs from a Tcl script in that modules must be compiled and then loaded, whereas scripts can be edited and loaded directly. Importantly, a module can be written to create new Eggdrop-specific Tcl commands and binds that a user can then use in a Tcl script. For example, the server module loaded by Eggdrop is what creates the "jump" Tcl command, which causes tells the Eggdrop to jump to the next server in its server list.

There are a few required functions a module must perform in order to properly work with Eggdrop

Module Header
-------------

A module should include license information. This tells other open source users how they are allowed to use the code. Eggdrop uses GPL 2.0 licensing, and our license information looks like this::

  /*
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

Required Code
-------------

For this section, you don't necessarily need to understand what it is doing, but this code is required for a module to function. If you want to learn more about this, check out :ref:`writing_module`

You'll next want to name your module::

  #define MODULE_NAME "woobie"

Declare your own function tables (again, you don't need to understand this part; you just need to copy/paste it)::

  #undef global
  static Function *global = NULL, *server_funcs = NULL;
  EXPORT_SCOPE char *woobie_start();

Next are two memory-related functions used by the core Eggdrop .status and .module commands::

  static int woobie_expmem()
  {
    int size = 0;

    return size;
  }

  static void woobie_report(int idx, int details)
  {
    if (details) {
      int size = woobie_expmem();

      dprintf(idx, "    Using %d byte%s of memory\n", size,
              (size != 1) ? "s" : "");
    }
  }

This function is called when Eggdrop loads the module::

  char *woobie_start(Function *global_funcs)
  {
    global = global_funcs;

    /* Register the module. */
    module_register(MODULE_NAME, woobie_table, 2, 1);
    /*                                            ^--- minor module version
     *                                         ^------ major module version
     *                           ^-------------------- module function table
     *              ^--------------------------------- module name
     */

    if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.8.0 or later.";
    }

This next function is used to unload the module::

  static char *woobie_close()
  {
    module_undepend(MODULE_NAME);
    return NULL;
  }

This creates a function table that is exported to Eggdrop. In other words, these are commands that are made available to the Eggdrop core and other modules. At minimum, the following functions must be exported::

  static Function woobie_table[] = {
    (Function) woobie_start,
    (Function) woobie_close,
    (Function) woobie_expmem,
    (Function) woobie_report,
  };

At this point, you should have a module that compiles and can be loaded by Eggdrop- but doesn't really do anything yet. We'll change that in the next section!

Adding a Partyline Command
--------------------------

A partyline command function accepts three arguments- a pointer to the user record of the user that called the command; the idx the user was on when calling the command; and a pointer to the arguments appended to the command. A command should immediately log that it was called to the LOG_CMDS log level, and then run its desired code. This simple example prints "WOOBIE" to the partyline idx of the user that called it::

  static int cmd_woobie(struct userrec *u, int idx, char *par)
  {
    putlog(LOG_CMDS, "*", "#%s# woobie", dcc[idx].nick);
    dprintf(idx, "WOOBIE!\n");
    return 0;
  }

If you add partyline commands, you need to create a table which links the new command name to the function it should call. This can be done like so::

  static cmd_t mywoobie[] = {
    /* command  flags  function     tcl-name */
    {"woobie",  "",    cmd_woobie,  NULL},
    {NULL,      NULL,  NULL,        NULL}  /* Mark end. */
  };

The tcl-name field can be a name for a Tcl command that will also call the partyline command, or it can be left as NULL.

Adding a Tcl Command
--------------------

Eggdrop uses the Tcl C API library to interact with the Tcl interpreter. Learning this API is outside the scope of this tutorial, but this example Tcl command will echo the provided argument::


  static int tcl_echome STDVAR {
    BADARGS(2, 2, " arg");

    if (strcmp(argv[1], "llama") {
      Tcl_AppendResult(irp, "You said: ", argv[1], NULL);
      return TCL_OK;
    } else {
      Tcl_AppendResult(irp, "illegal word!");
      return TCL_ERROR;
    }
  }

  A few notes on this example. BADARGS is a macro that checks the input provided to the Tcl command. The first argument BADARGS accepts is the minimum number of parameters the Tcl command must accept (including the command itself). The second argument is the maximum number of parameters that BADARGS will accept. The third argument is the help text that will be displayed if these boundaries are exceeded. For example, BADARGS(2, 4, " name ?date? ?place?") requires at least one argument to be passed, and a maximum of three arguments. Eggdrop code style is to enclose optional arguments between qusetion marks in the help text.

Similar to adding a partyline command, you also have to create a function table for a new Tcl command::

  static tcl_cmds mytcl[] = {
    {"echome",           tcl_echome},
    {NULL,                   NULL}   /* Required to mark end of table */
  };

And now the newly-created Tcl command 'echome' is available for use in a script!

Adding a Tcl Bind
-----------------

A Tcl bind is a command that is activated when a certain condition is met. With Eggdrop, these are usually linked to receiving messages or other IRC events. To create a bind, you must first register the bind type with Eggdrop when the module is loaded (you added the woobie_start() and woobie_close functions earlier, you still need all that earlier code in here as well)::

  static p_tcl_bind_list H_woob;

  ...

  char *woobie_start(Function *global_funcs)
  {
    ...
    H_woob = add_bind_table("woobie", HT_STACKABLE, woobie_2char);  
  }

And then remove the binds when the module is unloaded::

  static char *woobie_close()
  {
    ...
    del_bind_table(H_woob);
  }

Here, "woobie" is the name of the bind (similar to the PUB, MSG, JOIN types of binds you already see in tcl-commands.doc). HT_STACKABLE means you can have multiple binds of this type. "woobie_2char" defines how many arguments the bind will take, and we'll talk about that next.

Defining bind arguments
^^^^^^^^^^^^^^^^^^^^^^^

The following code example defines a bind that will take two arguments::

  static int woobie_2char STDVAR
  {
    Function F = (Function) cd;

    BADARGS(3, 3, " nick chan");

    CHECKVALIDITY(woobie_2char);
    F(argv[1], argv[2]);
    return TCL_OK;
  }

And this example defines a bind that will take three arguments::

  static int woobie_3char STDVAR
  {
    Function F = (Function) cd;

    BADARGS(4, 4, " foo bar moo");

    CHECKVALIDITY(woobie_3char);
    F(argv[1], argv[2], argv[3]);
    return TCL_OK;
  }

Like before, BADARGS still checks that the number of arguments passed is correct, and outputs help text if it is not. The rest is boilerplate code to pass the arguments when the bind is called.

Calling the Bind
^^^^^^^^^^^^^^^^

To call the bind, Eggdrop coding style it to name that function "check_tcl_bindname". So here, whenever we reach a point in code that should trigger the bind, we'll call check_tcl_woobie() and pass the arguments we defined- in this case, two arguments that woobie_2char was created to handle. Here is some sample code::

  check_tcl_woobie(chan, nick);


  static int check_tcl_woobie(char *chan, char *nick, char *userhost) {
    int x;
    char mask[1024];
    struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

    snprintf(mask, sizeof mask, "%s %s!%s",
                                  chan, nick, userhost);
    Tcl_SetVar(interp, "_woob1", nick ? (char *) nick : "", 0);
    Tcl_SetVar(interp, "_woob2", chan, 0);
    x = check_tcl_bind(H_woob, mask, &fr, " $_woob1 $_woob2",
          MATCH_MASK | BIND_STACKABLE);
    return (x == BIND_EXEC_LOG);
  }

Now that we have encountered a condition that triggers the bind, we need to check it against the binds the user has loaded in scripts and see if it matches those conditions. This is done with check_tcl_bind(), called with the bind type, the userhost of the user, the flag record of the user if it exists, the bind arguments, and bind options.

Exporting the Bind
------------------

Do we need to do this?
