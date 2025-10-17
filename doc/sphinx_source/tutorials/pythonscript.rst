================================
Writing an Eggdrop Python Script
================================

So you want to write an Eggdrop python script, but you don't really know where
to begin. This file will give you a very basic idea about what Eggdrop python
scripting is like, using a *very* simple script that may help you get
started with your own scripts.

This guide assumes you know a bit about Eggdrops and, because it reimplements those commands, Tcl. You should have
already installed Eggdrop. The bot should not be on any important or busy
channels (development bots can be annoying if your script has bugs). If you
plan on doing a lot of development, enable the .python commands, and
make sure nobody else has access to your bot.

First, read through the script below. Very few commands are listed here intentionally,
but as you want to develop more advanced scripts, you will definitely want to
get familiar with the `Eggdrop custom Tcl commands <https://docs.eggheads.org/using/tcl-commands.html>`_, as Python reimplements them.

If you have the .python command enabled, you can load a script by typing
'.pysource script/file.tcl' to load it. Otherwise, add it to your config
file like normal (examples to do so are at the bottom of the config file) and
'.rehash' or '.restart' your bot.

Let's look at a very basic example script that greets users as they join a channel::

  from eggdrop import bind
  from eggdrop.tcl import putmsg

  def joinGreetUser(nick, host, handle, channel, **kwargs):
    putmsg(channel, f"Hello {nick}, welcome to {channel}")

  def joinGreetOp(nick, host, handle, channel, **kwargs):
    putmsg(channel, f"{nick} is an operator on this channel!")

  if 'GREET_BINDS' in globals():
    for greetbind in GREET_BINDS:
      greetbind.unbind()
    del GREET_BINDS

  GREET_BINDS = list()
  GREET_BINDS.append(bind("join", "*", "*", joinGreetUser))
  GREET_BINDS.append(bind("join", "o", "*", joinGreetOp))

  print('Loaded greet.py')

Whew! There's a lot going on here. You'll generally see scripts broken into a few key parts- the header, the config section, and the code section. Ok, let's go over this piece by piece. Any line prefixed by a # means it is comment, and thus ignored. You can type whatever you want, and it won't matter. When writing scripts (especially if you want to give them to other people, it is good to use comments in the code to show what you're doing.

Because developers are lazy and don't want to manually reimplement every single Tcl command in Python code, Python includes a Tcl module that calls Tcl command via Python::

  from eggdrop import bind
  from eggdrop.tcl import putmsg

You can load the entire Tcl library by running ``import eggdrop.tcl``, or (the better solution to save memory) is to only load the commands you need via something like ``from eggdrop.tcl import putmsg``. The one exception to be aware of here is the bind command- you always want to load the custom python implementation of that so that it called a python bind, not a Tcl proc. Do this via ``from eggdrop import bind``.

Next, let's create some functions that the main script will call::

  def joinGreetUser(nick, host, handle, channel, **kwargs):
    putmsg(channel, f"Hello {nick}, welcome to {channel}")

  def joinGreetOp(nick, host, handle, channel, **kwargs):
    putmsg(channel, f"{nick} is an operator on this channel!")

Above, we create a function called joinGreetUser and a function called joinGreetOp. This function calls the putmsg Tcl. Note that the arguments here are slightly different than Tcl, requiring a comma instead of a space between the arguments.

We include the next code segment due to a limitation of Eggdrop's Python module. Currently, if Eggdrop is rehashed, previously existing binds will be duplicated instead of being overwritten. This is example code checks for previously-existing binds after the script reloaded and deletes them::

  if 'GREET_BINDS' in globals():
    for greetbind in GREET_BINDS:
      greetbind.unbind()
    del GREET_BINDS

As part of the above code, we need to track the binds. To do that, the GREET_BINDS list is created, and then we add the binds to it::

  GREET_BINDS = list()
  GREET_BINDS.append(bind("join", "*", "*", joinGreetUser))
  # Again, arguments are separated with a comma. This bind requires the 'o' flag to be triggered.
  GREET_BINDS.append(bind("join", "o", "*", joinGreetOp))

Note that you must call binds at the end of a script (not the top, like we do in Tcl), because the functions must be defined before being called. And again, we create the binds using comma-separated arguments, not spaces like Tcl. ``GREET_BINDS`` is created as a JOIN bind that is triggered when a user with an operator flag joins. When it is triggered, it called the function joinGreetOp that we defined at the beginning of the script.

And lastly, our lovely ``print('Loaded greet.py')`` line. This is a normal python command, nothing special about it- it will only print to standard output (ie, if you're running in a terminal). If you want to print to something like the partyline, you'd want to import and then use ``putlog("Loaded greet.py``. 


Copyright (C) 2003 - 2025 Eggheads Development Team
