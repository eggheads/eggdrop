.. _installing_eggdrop:

=======================================
Installing Eggdrop
=======================================

This is the quick install guide; if you have had little or no experience
with UNIX or Eggdrop, READ THE README FILE NOW! This file is best for
experienced users.

For more information on compiling Eggdrop, see the Compile Guide in
doc/COMPILE-GUIDE (and of course, the README FILE).

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

4. Eggdrop must be installed in a directory somewhere. This is
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

5. Since version 1.8, Eggdrop can use SSL to protect botnet links. If you intend on protecting botnet traffic between Eggdrops, you must generate SSL certificates by running::

        make sslcert

     Or, if you installed your eggdrop to a different directory in step 4, you
     will want to run:

       make sslcert DEST=<directory>

    For those using scripts to install Eggdrop, you can non-interactively
    generate a key and certificate by running:

       make sslsilent

     Read docs/TLS for more info on this process.

[The following steps are performed in the directory you just installed Eggdrop into from the previous step]

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
     the bot should crash. Eggdrop includes a helper script to auto-generate either a systemd or crontab entry. To add a systemd job, run::

        ./scripts/autobotchk [yourconfig.conf] -systemd

    or to add a crontab job, run::

        ./scripts/autobotchk [yourconfig.conf]

10. Smile. You have an Eggdrop!

Cygwin Requirements (Windows)
----------------------------------------

Eggdrop requires the following packages to be added from the Cygwin
installation tool prior to compiling:

::

  Interpreters: tcl, tcl-devel
  Net:          openssl, libssl-devel
  Devel:        autoconf, gcc-core, git, make
  Libs:         zlib-devel

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

This is the end. If you read to this point, hopefully you have also read
the README file. If not, then READ IT!&@#%@!

Have fun with Eggdrop!

  Copyright (C) 1997 Robey Pointer
  Copyright (C) 1999 - 2024 Eggheads Development Team
