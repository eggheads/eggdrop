Compilation and Installation of Eggdrop
Last revised: July 24, 2004

=======================================
Installing Eggdrop
=======================================

This is the quick install guide; if you have had little or no experience
with UNIX or Eggdrop, READ THE README FILE NOW! This file is only for
experienced users.

For more information on compiling Eggdrop, see the Compile Guide in
doc/COMPILE-GUIDE (and of course, the README FILE).

Overview
--------
1. What is Eggdrop?
2. Quick Startup
3. Cygwin Requirements (Windows)
4. Modules
5. Frequently Asked Questions

What is Eggdrop?
----------------

Please, read the README file before attempting to set up this bot. This
file is a quick setup guide, not a miracle worker. If you enter this file
without basic Eggdrop knowledge, you will NOT leave with a working bot!
Before asking ANY questions, READ THE README FILE OR YOU WILL BE BURNED
TO A HORRIBLE DEATH! IF YOU DO NOT READ THAT FILE I WILL PERSONALLY WALK
TO YOUR TERMINAL AND BEAT IT WITH A SMELLY SNEAKER! By the way, read the
README file.

Quick Startup
-------------

Eggdrop uses the GNU autoconfigure scripts to make things easier.

1. Type './configure' from the Eggdrop directory. The configure script
     will determine how your system is set up and figure out how to
     correctly compile Eggdrop. It will also try to find Tcl, which is
     required to compile.

2. Type either 'make config' or 'make iconfig' to determine which
     modules will be compiled. 'make config' compiles the default modules
     (everything but woobie.mod). If you want to choose which modules to
     compile, use 'make iconfig'.

3. Type 'make' from the Eggdrop directory, or to force a statically
     linked module bot, type 'make static'. Otherwise, the Makefile will
     compile whatever type of bot the configure script determined your
     system will support. Dynamic is always the better way to go if
     possible. There are also the 'debug' and 'sdebug' (static-debug)
     options, which will give more detailed output on a (highly unlikely :)
     crash. This will help the development team track down the crash and
     fix the bug. Debug and sdebug will take a little longer to compile
     and will enlarge the binary a bit, but it's worth it if you want to
     support Eggdrop development.

4. Eggdrop must be installed in a directory somewhere.  This is
     accomplished by entering the UNIX command::

       make install

     This will install the Eggdrop in your home directory in a directory
     called 'eggdrop' (i.e. /home/user/eggdrop).

     If you want to install to a different directory, use::

           make install DEST=<directory>

     For example::

       make install DEST=/home/user/otherdir

     Note that you must use full path for every file to be correctly
     installed.

     [The following is performed from the directory installed above.]

5. By default, version 1.8 uses SSL to protect botnet links. If you intend
     on linking 1.8 bots together, you must run::

        make sslcert

     Or, if you installed your eggdrop to a different directory in step 4, you
     will want to run:

       make sslcert DEST=<directory>

     Read docs/TLS for more info on this process.

6. Edit your config file completely.

7. Start the bot with the "-m" option to create a user file, i.e. ::

       ./eggdrop -m LamestBot.conf

8. When starting the bot in the future, drop the "-m". If you have edited
     your config file correctly, you can type::

       chmod u+x <my-config-file-name>

     For example::

       chmod u+x LamestBot.conf

     From then on, you will be able to use your config file as a shell
     script. You can just type "./LamestBot.conf" from your shell prompt
     to start up your bot. For this to work, the top line of your script
     MUST contain the correct path to the Eggdrop executable.

9. It's advisable to run your bot via crontab, so that it will
     automatically restart if the machine goes down or (heaven forbid)
     the bot should crash. Look at 'scripts/botchk' and 'scripts/autobotchk'
     for a great start with crontabbing the bot.

10. Smile, and if you haven't already read the README file in its
    entirety, go take a long walk off a short pier.

Cygwin Requirements (Windows)
----------------------------------------

Eggdrop requires the following packages to be added from the Cygwin
installation tool prior to compiling:

::

  Interpreters: tcl, tcl-devel
  Net:          openssl-devel
  Devel:        autoconf, gcc-core, git, make
  Utils:        diffutils

Modules
-------

Modules are small pieces of code that can either be compiled into the
binary or can be compiled separately into a file. This allows for a much
smaller binary.

If there are any modules that you have made or downloaded, you can add
them to the bot by placing them in the /src/mod directory with a mod
extension. They will be automatically compiled during make for you.
They must have a valid Makefile and, of course, be compatible with
the rest of the Eggdrop source.

If you wish to add a module at a later time, follow the same steps in
paragraph 2. After you have moved the appropriate files, you will only
need to type 'make modules' to compile only the modules portion of the
bot.

FREQUENTLY ASKED QUESTIONS
--------------------------

    (Q) What do I do if...?

    (A) READ THE README FILE!

    (Q) The readme does not answer...!

    (A) READ THE README FILE AGAIN!

    (Q) I still don't know how to...

    (A) MEMORIZE THE README FILE!

    (Q) But...

    (A) Well, go to www.egghelp.org or www.eggheads.org and see if you can
        find there what you're looking for. There are also lots of IRC help
        channels and various mailing lists, as seen in the README FILE.

This is the end. If you read to this point, hopefully you have also read
the README file. If not, then READ IT!&@#%@!

Have fun with Eggdrop!

  Copyright (C) 1997 Robey Pointer
  Copyright (C) 1999 - 2019 Eggheads Development Team
