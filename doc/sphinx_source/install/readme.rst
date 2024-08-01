README
======

  Please, at least SKIM this document before asking questions. In fact,
  READ IT if you've never successfully set up an Eggdrop bot before.

Notice
------

    Make SURE that you select your +n (owner) users wisely. They have 100%
    access to your bot and account. ONLY GIVE THIS POWER TO SOMEONE YOU
    TRUST COMPLETELY!

What is Eggdrop?
----------------

    Eggdrop is the world's most popular Internet Relay Chat (IRC) bot; it is
    freely distributable under the GNU General Public License (GPL). Eggdrop
    is a feature-rich program designed to be easily used and expanded upon by
    both novice and advanced IRC users on a variety of hardware and software
    platforms.

    An IRC bot is a program that sits on an IRC channel and performs automated
    tasks while looking just like a normal user on the channel. Some of these
    functions include protecting the channel from abuse, allowing privileged
    users to gain op or voice status, logging channel events, providing
    information, hosting games, etc.

    One of the features that makes Eggdrop stand out from other bots is module
    and Tcl scripting support. With scripts and modules you can make the bot
    perform almost any task you want. They can do anything: from preventing
    floods to greeting users and banning advertisers from channels.

    You can also link multiple Eggdrop bots together to form a botnet. This
    can allow bots to op each other securely, control floods efficiently and
    even link channels across multiple IRC networks. It also allows the
    Eggdrops share user lists, ban/exempt/invite lists, and ignore
    lists with other bots if userfile sharing is enabled. This allows users
    to have the same access on every bot on your botnet. It also allows the
    bots to distribute tasks such as opping and banning users. See doc/BOTNET
    for information on setting up a botnet.

    Eggdrop is always being improved and adjusted because there are bugs to
    be fixed and features to be added (if the users demand them and they make
    actually sense). In fact, it existed for several years as v0.7 - v0.9
    before finally going 1.0. This version of Eggdrop is part of the 1.9 tree.
    A valiant effort has been made to chase down and destroy bugs.

    This README file contains information about how to get Eggdrop, command
    line options for Eggdrop, what you may need to do when upgrading from
    older versions, a list of frequently asked questions, how to set up a
    crontab, some boring legal stuff, some basics
    about git usage and some channels where you might get help with Eggdrop.

How to Get Eggdrop
------------------

There are two official methods to download Eggdrop source code. Alternately, Eggdrop also comes as a docker image.

FTP
^^^

  The latest Eggdrop stable source code is always located at `<https://geteggdrop.com>`_. You can also download the current stable, previous stable, and development snapshot at `<https://ftp.eggheads.org/pub/eggdrop/source>`_

Git Development Snapshot
^^^^^^^^^^^^^^^^^^^^^^^^

    Eggdrop development is based on git. If you are interested in trying out
    the VERY LATEST updates to Eggdrop, you may be interested in pulling the
    most recent code from there. BE WARNED, the development branch of Eggdrop
    is not to be considered stable and may (haha) have some significant bugs
    in it.

    To obtain Eggdrop via the git repository (hosted by GitHub), you can
    either clone the repository via git or download a development snapshot.

    To clone the repository, simply type::

      git clone https://github.com/eggheads/eggdrop.git 

    Otherwise, you can download the development snapshot as a tar archive
    from:

      `<https://github.com/eggheads/eggdrop/archive/develop.tar.gz>`_

Docker
^^^^^^

  You can pull the official Eggdrop Docker image via::

    docker pull eggdrop:latest

  Additional Eggdrop Docker documentation can be found at `<https://hub.docker.com/_/eggdrop>`_


System Pre-Requisites
---------------------

  Before you can compile Eggdrop, Tcl must be installed on your system. Many systems have Tcl installed on them by default (you can check by trying the command "tclsh"; if you are given a '%' for a prompt, it is, and you can type 'exit' to exit the Tcl shell. However, Eggdrop also requires the Tcl development header files to be installed. They can often be installed via an OS package manager, usually called something similar to 'tcl-dev' for the package name. You can also download Tcl source from `<https://www.tcl.tk/software/tcltk/download.html>`_. 

  Eggdrop also requires openssl (and its development headers) in order to enable SSL/TLS protection of network data. The header files are often called something similar to 'libssl-dev'. While not advised, this requirement can be removed by compilling using ``./configure --disable-tls``, but you will not be able to connect to TLS-protected IRC servers nor utilize secure botnet communication.

Minimum Requirements
--------------------

Some components of Eggdrop relies on a variety of third-party libraries, documented here.

+-------------------------------+-------------------+-------------------+
| Functionality                 | Package           | Minimum Version   |
+===============================+===================+===================+
| Tcl interpreter (required)    | Tcl Dev Library   | 8.5.0             |
+-------------------------------+-------------------+-------------------+
| Secure communication          | OpenSSL           | 0.9.8             |
+-------------------------------+-------------------+-------------------+
| Python module                 | Python            | 3.8.0             |
+-------------------------------+-------------------+-------------------+
| Compression module            | zlib              | Any               |
+-------------------------------+-------------------+-------------------+


Quick Startup
-------------

    Please see the `Install <install/install.html>`_ file after you finish reading this file.

Upgrading
---------

    The upgrade process for Eggdrop is very simple, simply download the new source code and repeat the compile process. You will want to read the NEWS for any new configuration file settings you want to add. Please see `Upgrading <install/upgrading.html>`_ for full details.

Command Line
------------

    Eggdrop has some command line options - not many, because most things
    should be defined through the config file. However, sometimes you may
    want to start up the bot in a different mode and the command line
    options let you do that. Basically, the command line for Eggdrop is::

     ./eggdrop [options] [config-file]

    The options available are:

      -t: Don't background, use terminal. Your console will drop into an
           interactive partyline session, similar to a DCC chat with the bot.
           This is useful for troubleshooting connection issues with the bot.

      -c: Don't background, show channel info. Every 10 seconds your screen
           will clear and you will see the current channel status, sort of
           like "top".

      -m: Create userfile. If you don't have a userfile, this will make Eggdrop
          create one and give owner status to the first person that introduces
          himself or herself to it. You'll need to do this when you first set
          up your bot.

      -h: Show help, then quit.

      -v: Show version info, then quit.

    Most people never use any of the options except -m and you usually only
    need to use that once.

Auto-starting Eggdrop
---------------------

Systems go down from time to time, taking your Eggdrop along with it. You may not be not around to restart it manually, so you can instead use features of the operating system to automatically restart Eggdrop should it quit for any reason. Eggdrop comes with an autobotchk shell script that can create either a systemd or crontab entry. The systemd option will monitor your Eggdrop and a) start it when the machine boots and b) restart the Eggdrop if it crashes for any reason. The (older) crontab option will check (by default) every 10 minutes to see if your Eggdrop is still running, and attempt to restart it if it is not.

    To auto-generate a systemd job, from the Eggdrop install directory, simply run::

      ./scripts/autobotchk <Eggdrop config file> -systemd

    To auto-geneerate a script to check Eggdrop's status and run it via a crontab entry, simply run::

      ./scripts/autobotchk <Eggdrop config file>

    This will crontab your bot using the default setup. If you want a list of autobotchk options, type './autobotchk'. A crontab example with options would be::

      ./scripts/autobotchk <Eggdrop config file> -noemail -5

    This would setup crontab to run the botchk every 5 minutes and not send you an email saying that it restarted your bot.

Documentation
-------------

    We're trying to keep the documentation up to date. If you feel that
    anything is missing here or that anything should be added, etc, please
    create an issue, or better yet a pull request, at 
    `<https://www.github.com/eggheads/eggdrop>`_ Thank you!

Obtaining Help
--------------

    You can obtain help with Eggdrop in the following IRC channels:

      * Libera Chat - #eggdrop (official channel), #eggheads (development discussion)
      * DALnet - #eggdrop
      * EFnet - #egghelp
      * IRCnet - #eggdrop
      * QuakeNet - #eggdrop.support
      * Undernet - #eggdrop

    If you plan to ask questions in any of the above channels, you should be
    familiar with and follow IRC etiquette:

      * Don't type using CAPITAL letters, colors or bold.
      * Don't use  "!" and "?" excessively.
      * Don't /msg people without their permission.
      * Don't repeat or paste more than 4 lines of text to the channel.
      * Don't ask to ask- just state your question, along with any relevant details and error messages

Copyright (C) 1997 Robey Pointer
Copyright (C) 1999 - 2024 Eggheads Development Team
