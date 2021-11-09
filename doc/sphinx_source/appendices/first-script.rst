Your First Eggdrop Script
Last revised: May 10, 2020

=========================
Your First Eggdrop Script
=========================

So you want to write an Eggdrop script, but you don't really know where
to begin. This file will give you a very basic idea about what Eggdrop
scripting is like, using a *very* simple script that may help you get
started with your own scripts.

This guide assumes you know a bit about Eggdrops and IRC. You should have
already installed Eggdrop. The bot should not be on any important or busy
channels (development bots can be annoying if your script has bugs). If you
plan on doing a lot of development, enable the .tcl and .set commands, and
make sure nobody else has access to your bot. The .tcl and .set commands
are helpful in debugging and testing your code.

First, read through the script. Very few commands are listed here intentionally,
but as you want to develop more advanced scripts, you will definitely want to
get familiar with the `core Tcl language commands <https://www.tcl.tk/man/tcl8.6/TclCmd/contents.htm>`_, especially the string- and list-related commands, as well as Eggdrop's own library of custom Tcl commands, located in `tcl-commands.doc <https://docs.eggheads.org/mainDocs/tcl-commands.html>`_

If you have the .tcl command enabled, you can load a script by typing
'.tcl source script/file.tcl' to load it. Otherwise, add it to your config
file like normal (examples to do so are at the bottom of the config file) and
'.rehash' or '.restart' your bot.

Let's look at a very basic example script that greets users as they join a channel::

  # GreetScript.tcl
  # Version: 1.0
  # Author: Geo <geo@eggheads.org>
  #
  # Description:
  # A simple script that greets users as they join a channel
  #
  ### Config Section ###
  # How would you like the bot to gree users?
  # 0 - via public message to channel
  # 1 - via private message
  set pmsg 0
  # What message would you like to send to users when they join?
  set greetmsg "Hi! Welcome to the channel!"
  ### DO NOT EDIT BELOW HERE UNLESS YOU KNOW WHAT YOU ARE DOING! ###

  bind join - * greet

  proc greet {nick uhost hand chan} {
    global pmsg
    global greetmsg
    if {$pmsg} {
      putserv "PRIVMSG $nick :$greetmsg"
    } else {
      putserv "PRIVMSG $chan :$greetmsg"
    }
  } 

  putlog "greetscript.tcl v1.0 by Geo"

Whew! There's a lot going on here. You'll generally see scripts broken into a few key parts- the header, the config section, and the code section. Ok, let's go over this piece by piece. First, the header of the script::

  # GreetScript.tcl
  # Version: 1.0
  # Author: Geo <geo@eggheads.org> or #eggdrop on Libera
  #
  # Description:
  # A simple script that greets users as they join a channel

Any line prefixed by a # means it is comment, and thus ignored. You can type whatever you want, and it won't matter. When writing scripts (especially if you want to give them to other people, it is good to use comments in the code to show what you're doing. Here though, we use it to describe what the script is and, most importantly, who wrote it! Let's give credit where credit is due, right? You may want to give users a way to contact you as well.

Next, let's look at the configuration section::

  ### Config Section ###
  # How would you like the bot to gree users?
  # 0 - via public message to channel
  # 1 - via private message
  set pmsg 0
  # What message would you like to send to users when they join?
  set greetmsg "Hi! Welcome to the channel!"
  ### DO NOT EDIT BELOW HERE UNLESS YOU KNOW WHAT YOU ARE DOING! ###

To make scripts easy to use, you'll want to have a section that allows users to easily change the way the script operates, without having to edit the script itself. Here, we have two settings: one that controls which method the Eggdrop uses to greet a user, and a second with the message to greet the user with. Sure, we could hard-code that into the code section below, but in larger scripts that makes things harder to find, and also forces you to potentially have to make the same change multiple times in code. Why not make it simple and do it once, up top? Notice the settings do not have #s in front of them- they are variables that will be used by the script later on. And of course, the standard ominous warning not to change anything below!

Now, let's look start to dissect the actual code!::

  bind join - * greet

This is a bind. This sets up an action that Eggdrop will react to. You can read `all the binds that Eggdrop uses here. <https://docs.eggheads.org/mainDocs/tcl-commands.html>`_ Generally, we like to place all binds towards the top of the script so that they are together and easy to find. Now, let's look at documentation of the bind join together.

+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| bind JOIN                                                                                                                                                |
+============================+=============================================================================================================================+
| bind join <flags> <mask> <proc>                                                                                                                          |
+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| procname <nick> <user@host> <handle> <channel>                                                                                                           |
+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Description: triggered by someone joining the channel. The mask in the bind is matched against "#channel nick!user@host" |br| and can contain wildcards. |
+----------------------------------------------------------------------------------------------------------------------------------------------------------+

So here, we can start to match the bind listed in the code to how it is described in the documentation. The first term after the bind command is 'join', showing that it is a join bind, which means the action we define here will take place each time a user joins a channel. The next term is 'mask', and it says it is in the form "#channel nick!user@host". This is where we can start to refine exactly when this bind is triggered. If we want it to work for every person joining every channel Eggdrop is on, then a simple '*' will suffice here- that will match everything. If we wanted this bind to only work in #foo, then the mask would be "#foo \*". If we wanted to greet users on every channel, but only those who are on AOL, the mask would be "* \*!*@*.aol.com". Finally the 'proc' argument is the name of the Tcl proc we want to call, where the code that actually *does stuff* is located. 

So to sum up this line from the example script: When a user joins on any channel and with any hostmask, run the code located in proc 'greet'.

Now that we told the Eggdrop what action to look for, we need to tell it what to do when that action occurs!::

  proc greet {nick uhost hand chan} { 

This is how we declare a Tcl proc. As we said above, this is where the magic happens. To set up the proc (this will look differently for different binds), lets refer back to the bind JOIN documentation. The second line shows ``procname <nick> <user@host> <handle> <channel>``. Eggdrop does a lot of stuff in the background when a bind is triggered, and this is telling you how Eggdrop will present that information to you. Here, Eggdrop is telling you it is going to pass the proc you created four variables: One that contains the nickname of the person who triggered the bind (in this case, the user who joined), the user@host of that user, the handle of that user (if the user has one on the bot), and the channel that the bind was triggered on. 

So let's say someone with the nickname Geo with a hostmask of awesome@aol.com joined #sexystuff and that person is not added to the bot as a user. Eggdrop will pass 4 values to the variables you set up in that proc: The first variable will get the value "Geo", the second "awesome@aol.com", the third "*", and the fourth "#sexystuff". (That third value was a trick, we didn't talk about that- if the user is not added to the bot, handle will get a "*" as a value). Now, let's use those variables!::

  global pmsg
  global greetmsg

This is a simple one- because we're using variables declared in the main body of the script (remember way up top?), we have to tell this proc to use that variable, not not create a new local variable for this proc.

And finally, let's actually send a message to the user::

  if {pmsg}
    putserv "PRIVMSG $nick :$greetmsg"
  } else {
    putserv "PRIVMSG $chan :$greetmsg"
  }

Here, we're going to check if pmsg is true (any value that is not 0) and, if yes, send a private message to the user. If pmsg is not true (it is 0), then we will send the message to the channel. You can see that the first putserv message sends a PRIVMSG message to $nick - this is the nickname of the user that joined, and that Eggdrop stored for us in the first variable of the proc, which we called 'nick'. The second putserv message will send a PRIVMSG message to the $chan - this is the channel the user joined, and that Eggdrop stored for us in the fourth variable of the proc, which we called 'chan'. 

And finally: get the credit you deserve when the script loads!::

  putlog "greetscript.tcl v1.0 by Geo"

Like your variables at the top of the script, this line is not inside a Tcl proc and will execute when the script is loaded. You can put this or any other initialization code you want to run.

And there you have it- your first script! Take this, modify it and experiment. A few challenges for you:

* How can you configure which channel it should run on, without hard-coding it into the bind? (Maybe with a variable...)
* How can you configure it to only message a user with the nickname "FancyPants"? (Sounds like something a bind could handle)
* How can you delay the message from sending by 5 seconds? (Hint: utimer)
* How can you send different messages to different channels? (A new setting may be in order...)
* How can you get the bot to not greet itself when it joins the channel? (Eggdrop stores its own nickname in a variable called $botnick)
* How can you add the person joining the channel's nickname to the greet message? (You can put variables inside variables...)

If you want to try these out, join #eggdrop on Libera and check your answers (and remember, there are dozens of ways to do this, so a) don't be defensive of your choices, and b) take others' answers with a grain of salt!)

.. |br| raw:: html

    <br>

Copyright (C) 2003 - 2021 Eggheads Development Team
