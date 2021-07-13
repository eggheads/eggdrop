:orphan:

Setting up your Eggdrop the first time
======================================

*This guide was based off perhaps the most helpful Eggdrop website ever written, egghelp.org by slennox. As happens with life, slennox has moved on and no longer updates it, but the information here is still incredibly useful. We have updated his setup page here and hope it will continue to prove useful to our users*

Prerequisites
-------------

Make sure Tcl AND it's dev packages are installed on your system. On Debian-based systems, this is done with::

    sudo apt-get install tcl tcl-dev

It is also STRONGLY recommended to have openssl installed, to enable SSL/TLS protection::

    sudo apt-get install openssl libssl-dev

The super-short version
-----------------------

You can read the `Installation`_ section for a more detailed explanation of these steps.

1. Download the `latest stable Eggdrop release <https://ftp.eggheads.org/pub/eggdrop/source/stable/eggdrop-1.9.1.tar.gz>`_ to your shell via FTP, or simply type ``wget geteggdrop.com -O eggdrop-1.9.1.tar.gz``
2. From the commadline of your shell, type ``tar zxvf eggdrop-1.9.1.tar.gz``
3. Type ``cd eggdrop-1.9.1``
4. Type ``./configure``
5. Type ``make config``
6. Type ``make``
7. Type ``make install``
8. Type ``cd ~/eggdrop``
9. For a quick start, edit the eggdrop-basic.conf file. To take advantage of all Eggdrop's features, we recommend using eggdrop.conf instead. It is also a good idea to rename the file to something easy to remember and specific to your bot, like botnick.conf.
10. Type ./eggdrop -m <config file>

Getting the source
------------------

History
~~~~~~~

There are two major versions of Eggdrop currently in use- 1.9.x and 1.8.x. The 1.6 series, while still popular, is no longer supported by the developers.

The most current version of Eggdrop, and the one appropriate for most users, is the current 1.9 series. It added many features such as SASL support, multi-ip listening, and a new password hashing module. It is the most complete, feature-rich, and functional version of Eggdrop. If you're just starting out with Eggdrop, you should use 1.9.1

Prior to that, the 1.8 series added several major features, to include IPv6 support and SSL/TLS connections. 1.6.21, which is now over 10 years old, was the last release of the 1.6 series and is still used by users who have become comfortable with that version and may have spent much time applying their own modifications to make it work the way they want, and therefore don't wish to move to a newer version. The majority of Tcl scripts out there were written for 1.6 bots, but those scripts usually work on 1.8 and 1.9 bots as well.

The 1.9 Eggdrop tree is currently under active development and the most recent changes are available in daily snapshots for users to download for testing. While the development snapshot will contain the most current, up-to-date features of Eggdrop, it is not yet considered stable and users stand a higher chance of encountering bugs during use. If you do use it and find a bug, it is highly encouraged to report it via the `Eggheads GitHub issues page. <https://github.com/issues>`_

Download locations
~~~~~~~~~~~~~~~~~~

The developers distribute Eggdrop via two main methods: FTP, and GitHub. For FTP, it is packaged in tarball format (with the .tar.gz filename extension), with the version number in the filename. The Eggdrop 1.9.0 source, for example, would be named eggdrop-1.9.0.tar.gz.

`The Eggheads FTP <https://ftp.eggheads.org/pub/eggdrop/>`_ is a repository for the `current <https://ftp.eggheads.org/pub/eggdrop/source/1.9/eggdrop1.9.1.tar.gz>`_ version of Eggdrop, as well as the most current development snapshot and previous stable releases.

Eggdrop also maintains a `GitHub page <https://github.com/eggheads/eggdrop>`_ where you can download the development snapshot or a stable version, via either git commandline or by downloading a tarball. To download via git, type ``git clone https://github.com/eggheads/eggdrop.git``, then ``cd eggdrop``. This gives you the development version. To switch to the most recent stable version, type ``git checkout stable/1.9``. You can then skip to step 4 in the Installation section below.

Installation
------------

Installing Eggdrop is a relatively simple process provided your shell has the required tools for successful compilation. On most commercial shell accounts which allow Eggdrop bots you won't have any problems with installation, but on some private boxes or a shell on your ISP you may experience errors during compilation.

Below is a step by step guide to the installation process. These instructions apply to 1.8 bots. It assumes you will be installing eggdrop-1.9.1.tar.gz, so just change the numbers if you are installing another version.

1. Put the Eggdrop source on your shell using one of the specified download locations, either by downloading the `eggdrop-1.9.1.tar.gz <https://ftp.eggheads.org/pub/eggdrop/source/1.9/eggdrop-1.9.1.tar.gz>`_ file to your system then uploading it to the shell via FTP, or downloading it directly to the shell via the shell's FTP client, git, wget, or curl. You don't need to put the .tar.gz file in its own directory (it'll be done automatically in the next step).

2. SSH to the shell (if you haven't already), and type ``tar zxvf eggdrop-1.9.1.tar.gz`` (if this doesn't work, try ``gunzip eggdrop-1.9.1.tar.gz`` then ``tar xvf eggdrop-1.9.1.tar``). This will extract the Eggdrop source into its installation directory, named 'eggdrop-1.9.1'.

3. Type cd eggdrop-1.9.1 to switch to the directory the Eggdrop source was extracted to.

4. Type ``./configure`` (that's a period followed by a slash followed by the word 'configure').  This makes sure the shell has all the right tools for compiling Eggdrop, and helps Eggdrop figure out how to compile on the shell.

5. When configure is done, type ``make config``. This sets up which modules are to be compiled. For a more efficient installation, you can use ``make iconfig`` to select the modules to compile, but if you're not sure just use make config.

6. Type ``make``. This compiles the Eggdrop. The process takes a brief moment on fast systems, longer on slow systems.

7. Type ``make install DEST=~/botdir``. This will install Eggdrop into a directory named 'botdir' in your home directory. You can change 'botdir' to anything you like. Note that in some cases you may need to specify the full path, e.g. ``make install DEST=/home/cooldude/botdir``, using the ~ character in make install won't always work. You can get the full path by typing ``pwd``.

8. You can safely delete the installation directory named 'eggdrop-1.9.1' (to do this, type ``cd ~`` then ``rm -rf eggdrop-1.9.1``) that was created previously, although some people may find it handy to keep that directory for performing additional or future installations of the same version without recompiling.

That's it! Eggdrop is now installed into its own directory on the shell. It's time to edit the configuration files to make Eggdrop work the way you want it to.

Configuration
-------------

You will need to edit the configuration file before you can start up your Eggdrop. You can find the example configuration file in the directory you extracted the Eggdrop source to, under the name 'eggdrop.conf'. If you downloaded Eggdrop to your system, you can unzip the tarball (.tar.gz) file to its own directory using 7-Zip or a similar program, and view the example config file, botchk file, and all the documentation files locally. You can use Notepad to edit these files, although it's sometimes desirable to use an editor that supports the Unix file format such as EditPlus. To edit the file once it is on your shell, a program such as 'nano' or 'vim' is recommended.

Editing the config file
~~~~~~~~~~~~~~~~~~~~~~~

Eggdrop comes with two versions of the configuration file- eggdrop.conf and eggdrop-basic.conf. While it is recommended that users edit a copy of eggdrop.conf to take advantage of all the features Eggdrop has to offer, using eggdrop-basic.conf to start will be a quicker path for some. Still, it is recommended that you come back to the full config file at some point to see what you are missing.

It is first recommended to rename the sample config to something other than "eggdrop.conf". Giving it the name of the bot's nick (e.g. NiceBot.conf) is quite common. In the config file, you set up the IRC servers you want the bot to use and set Eggdrop's options to suit your needs. Eggdrop has many options to configure, and editing the configuration file can take some time. I recommend you go over the entire config file to ensure the bot will be configured properly for your needs. All of the options in the config file have written explanations - be sure to read them carefully. Some of them can be a little bit vague, though.

To comment out a line (prevent the bot from reading that line), you can add a '#' in front of a line. When you come to a line that you need to edit, one popular option is to comment out the original and add your new line right below it. This preserves the original line as an example. For example::

	# Set the nick the bot uses on IRC, and on the botnet unless you specify a
	# separate botnet-nick, here.
	#set nick "Lamestbot"
	set nick LlamaBot

Below are some of the common settings used for Eggdrop:

:set username: if your shell runs identd (most do), then you should set this to your account login name.

:set vhost4: you'll need to set this if you want your bot to use a vhost. This setting lets you choose which IP to use if your shell has multiple. Use vhost4 for an IPv4 address (ie, 1.2.3.4) See also: vhost6

:set vhost6: the same as vhost4, only for IPv6 addresses (ie, 5254:dead:b33f::1337:f270).

:logfile: keeping logs is a good idea. Generally, you should have one log for bot stuff, and one log for each of your channels. To capture bot stuff, add the line ``logfile mcobxs * "botnick.log"`` to the config. To capture channel stuff, add ``logfile jkp #donkeys "#donkeys.log"``, ``logfile jkp #horses "#horses.log"``, etc. Make sure you remove the sample logfile lines for the channel #lamest. If you'd like to put your logfiles in their own directory, specify the directory in the log name (e.g. ``logfile jkp #donkeys "logs/#donkeys.log"`` to write the logfiles in the /logs directory).

:listen 3333 all: you will almost certainly want to change this, as 3333 will probably be in use if there are other Eggdrops running on the machine. Generally, you can choose any port from 1024 to 65535, but the 49152-65535 range is best as these are the private/dynamic ports least likely to be reserved by other processes. You can choose not to have a port by commenting this line out, but that will prevent any telnet connections to the bot (you won't be able to use the bot as a hub, won't be able to telnet to the bot yourself, and the bot won't respond to /ctcp botnick CHAT requests).

:set protect-telnet: setting this to 1 is strongly recommended for security reasons.

:set require-p: this is a useful feature allowing you to give party line access on a user-specific basis. I recommend setting it to 1.

:set stealth-telnets: when you telnet to your bot, it will usually display the bot's nickname and version information. You probably don't want people seeing this info if they do a port scan on the bot's shell. Setting this to 1 will prevent the bot from displaying its nickname and version when someone telnets to it.

:set notify-newusers: set this to the nick you will have on the bot. This setting isn't really used if you have learn-users switched off.

:set owner: you should only put one person in this list - yourself. Set it to the nick you will have on the bot. Do NOT leave it set to the default "MrLame, MrsLame".

:set default-flags: these are the flags automatically given to a user when they introduce themselves to the bot (if learn-users is on) or when they're added using .adduser. If you don't want the user to be given any flags initially, set this to "" or "-".

:set must-be-owner: if you have the .tcl and .set commands enabled, you should definitely set this to 1. In 1.3.26 and later, you can set it to 2 for even better security.

:set chanfile: the chanfile allows you to store 'dynamic' channels so that the bot rejoins the channel if restarted. Dynamic channels are those you make the bot join using the .+chan command - they aren't defined in the config file. The chanfile is good if you frequently add/remove channels from the bot, but can be a pain if you only like to add/remove channels using the config file since settings stored in the chanfile with overwrite those set in the config. You can choose not to use a chanfile by setting it to "".

:set nick: this is what you use to specify your bot's nickname. I recommend against using [ ] { } \ character's in the bot's nick, since these can cause problems with some Tcl scripts, but if you'd like to use them, you'll need to precede each of those characters with a backslash in the setting, e.g. if you wanted your bot to have the nick [NiceBot], use ``set nick "\[NiceBot\]"``.

:set altnick: if you want to use [ ] { } \ characters in the bot's alternate nick, follow the backslash rule described previously.

:set servers: you should specify multiple servers in this list, in case the bot is unable to connect to the first server. The format for this list is shown below: 

.. code-block:: tcl

  set servers {
      you.need.to.change.this:6667
      another.example.com:7000:password
      [2001:db8:618:5c0:263::]:6669:password
      ssl.example.net:+6697
  }

:set learn-users: this is an important setting that determines how users will be added to your Eggdrop. If set to 1, people can add themselves to the bot by sending 'hello' to it (the user will be added with the flags set in default-flags). If set to 0, users cannot add themselves - a master or owner must add them using the .adduser command.

:set dcc-block: although the example config file recommends you set this to 0 (turbo-dcc), this may cause DCC transfers to abort prematurely. If you'll be using DCC transfers a lot, set this to 1024.

Finally, be sure to remove the 'die' commands from the config (there are two of them 'hidden' in various places), or the bot won't start. Once you've finished editing the config file, make sure you rename it to something other than
"eggdrop.conf" if you haven't already. Then, if you edited the config file locally, upload the config file to the directory you installed the bot.

Starting the Eggdrop
--------------------

Phew! Now that you've compiled, installed, and configured Eggdrop, it's time to start it up. Switch to the directory to which you installed the bot, cross your fingers, and type ``./eggdrop -m <config>`` (where <config> is the name you gave to the config file). Eggdrop should start up, and the bot should appear on IRC within a few minutes. The -m option creates a new userfile for your bot, and is only needed the first time you start your Eggdrop. In future, you will only need to type ./eggdrop <config> to start the bot. Make sure you take the time to read what it tells you when you start it up!

Once your bot is on IRC, it's important that you promptly introduce yourself to the bot. Msg it the 'hello' command you specified in the config file, e.g. ``/msg <botnick> hello``. This will make you the bot's owner. Once that's done, you need to set a password using ``/msg <botnick> pass <password>``. You can then DCC chat to the bot.

Now that your Eggdrop is on IRC and you've introduced yourself as owner, it's time to learn how to use your Eggdrop!

No show?
~~~~~~~~

If your bot didn't appear on IRC, you should log in to the shell and view the bot's logfile (the default in the config file is "logs/eggdrop.log"). Note that logfile entries are not written to disk immediately unless quick-logs is enabled, so you may have to wait a few minutes before the logfile appears, or contains messages that indicate why your bot isn't showing up.

Additionally, you can kill the bot via the command line (``kill pid``, the pid is shown to you when you started the bot or can be viewed by running ``ps x``) and then restart it with the -mnt flag, which will launch you directly into the partyline, to assist with troubleshooting. Note that if you use the -nt flag, the bot will not persist and you will kill it once you quit the partyline.

If you're still unsure what the problem is, try asking in #eggdrop on Libera, and be sure to include any relevant information from the logfile. Good luck!

First steps with a running Eggdrop
==================================

Log on to the partyline
-----------------------
Now that your bot is online, you'll want to join the partyline to further use the bot. First, read what it tells you when you started it up::

  STARTING BOT IN USERFILE CREATION MODE.
  Telnet to the bot and enter 'NEW' as your nickname.
  OR go to IRC and type:  /msg BotNick hello
  This will make the bot recognize you as the master.

You can either telnet to the bot, or connect to the bot using DCC Chat. To telnet, you'll either need a program like Putty (Windows), or you can do it from the command line of your shell using the telnet command::

  telnet <IP of bot> <listen port>

You can find the IP and port the bot is listening on by a) remembering what you set in the config file ;) or b) reading the display the bot presented when it started up. Look for a line that looks similar to this::

  Listening for telnet connections on 2.4.6.9:3183 (all).

This tells you that the bot is listening on IP 2.4.6.9, port 3183. If you see 0.0.0.0 listed, that means Eggdrop is listening on all available IPs on that particular host.


If you choose not to telnet to connect to the partyline, you can either ``/dcc chat BotNick`` or ``/ctcp BotNick chat``. If one of those methods does not work for you, try the other. Once you're on the bot for the first time, type ``.help`` for a short list of available commands, or ``.help all`` for a more thorough list.

Common first steps
------------------

To learn more about any of these commands, type .help <command> on the partyline. It will provide you the syntax you need, as well as a short description of how to use the command.

To tell the Eggdrop to join a channel, use::

  .+chan #channel

To register a user with the bot, use::

  .+user <handle> 

The handle is the name that the bot uses to track a user. No matter what nickname on IRC a user uses, a single handle is used to track the user by their hostmask. To add a hostmask of a user to a handle, use::

  .+host <handle> <hostmask>

where the hostmask is in the format of <nick>!<ident>@hostname.com . Wildcards can be used; common formats are \*!\*@hostname.com for static hosts, or \*!ident@*.foo.com for dynamic hostnames.

To assign an access level to a user, first read ``.help whois`` for a listing of possible access levels and their corresponding flags. Then, assign the desired flag to the user with::

  .chattr <+flag> <handle>

So to grant a user the voice flag, you would do::

  .chattr +v handle

It is important to note that, when on the partyline, you want to use the handle of the user, not their current nickname.

Finally, Eggdrop is often used to moderate and control channels. This is done via the ``.chanset`` command. To learn more about the (numerous!) settings that can be used to control a channel, read::

  .help chaninfo

Common uses involve setting channels modes. This can be done with the chanmode channel setting::

  .chanset #channel chanmode +snt

which will enforce the s, n, and t flags on a channel.

Automatically restarting an Eggdrop
-----------------------------------

A common question asked by users is, how can I configure Eggdrop to automatically restart should it die, such as after a reboot? To do that, we use the system's crontab daemon to run a script (called botchk) every ten minutes that checks if the eggdrop is running. If the eggdrop is not running, the script will restart the bot, with an optional email sent to the user informing them of the action. To make this process as simple as possible, we have included a script that can automatically configure your crontab and botchk scripts for you. To set up your crontab/botchk combo:

1. Enter the directory you installed your Eggdrop to. Most commonly, this is ~/eggdrop (also known as /home/<username>/eggdrop).

2. Just humor us- run ``./scripts/autobotchk`` without any arguments and read the options available to you. They're listed there for a reason!

3. If you don't want to customize anything via the options listed in #2, you can start the script simply by running::

    ./scripts/autobotchk yourEggdropConfigNameHere.conf

4. Review the output of the script, and verify your new crontab entry by typing::

    crontab -l

By default, it should create an entry that looks similar to::

    0,10,20,30,40,50 * * * * /home/user/bot/scripts/YourEggdrop.botchk 2>&1

This will run the generated botchk script every ten minutes and restart your Eggdrop if it is not running during the check. Also note that if you run autobotchk from the scripts directory, you'll have to manually specify your config file location with the -dir option. To remove a crontab entry, use ``crontab -e`` to open the crontab file in your system's default editor and remove the crontab line.

Authenticating with NickServ
----------------------------

Many IRC features require you to authenticate with NickServ to use them. You can do this from your config file by searching for the line::

    #  putserv "PRIVMSG NickServ :identify <password>"

in your config file. Uncomment it by removing the '#' sign and then replace <password> with your password. Your bot will now authenticate with NickServ each time it joins a server.

Setting up SASL authentication
------------------------------

Simple Authentication and Security Layer (SASL) is becoming a prevalant method of authenticating with IRC services such as NickServ prior to your client finalizing a connection to the IRC server, eliminating the need to /msg NickServ to identify yourself. In other words, you can authenticate with NickServ and do things like receive a cloaked hostmask before your client ever appears on the IRC server. Eggdrop supports three methods of SASL authentication, set via the sasl-mechanism setting:

* **PLAIN**: To use this method, set sasl-mechanism to 0. This method passes the username and password (set in the sasl-username and sasl-password config file settings) to the IRC server in plaintext. If you only connect to the IRC server using a connection protected by SSL/TLS this is a generally safe method of authentication; however you probably want to avoid this method if you connect to a server on a non-protected port as the exchange itself is not encrypted.

* **ECDSA-NIST256P-CHALLENGE**: To use this method, set sasl-method to 1. This method uses a public/private keypair to authenticate, so no username/password is required. Not all servers support this method. If your server does support this, you you must generate a certificate pair using::

    openssl ecparam -genkey -name prime256v1 -out eggdrop-ecdsa.pem

  You will need to determine your public key fingerprint by using::

    openssl ec -noout -text -conv_form compressed -in eggdrop-ecdsa.pem | grep '^pub:' -A 3 | tail -n 3 | tr -d ' \n:' | xxd -r -p | base64

  Then, authenticate with your NickServ service and register your public certificate with NickServ. You can view your public key  On Libera for example, it is done by::

    /msg NickServ set pubkey <fingerprint string from above goes here>

* **EXTERNAL**: To use this method, set sasl-method to 2. This method allows you to use other TLS certificates to connect to the IRC server, if the IRC server supports it. An EXTERNAL authentication method usually requires you to connect to the IRC server using SSL/TLS. There are many ways to generate certificates; one such way is generating your own certificate using::

    openssl req -new -x509 -nodes -keyout eggdrop.key -out eggdrop.crt

You will need to determine yoru public key fingerprint by using::

    openssl x509 -in eggdrop.crt -outform der | sha1sum -b | cut -d' ' -f1

Then, ensure you have those keys loaded in the ssl-privatekey and ssl-certificate settings in the config file. Finally, to add this certificate to your NickServ account, type::

    /msg NickServ cert add <fingerprint string from above goes here>

Advanced Eggdrop Usage
======================

UTF-8 Support
-------------
The encoding scheme used by Eggdrop's Tcl interface is set based on the locale settings of the host machine. You can check which locale your host machine is using by running the ``locale`` command. Eggdrop takes that locale setting of the host machine and compares it to the locales available within Tcl's installed libraries. If it finds one in Tcl that matches (or is close to matching), that is the encoding scheme that is used. If a matching encoding scheme is not found, only then does eggdrop default to ISO 8859-1 encoding.

If you want Eggdrop to use a specific encoding scheme that it is not currently using, you can view the available locales on your machine via the ``locale -a`` command, and then set the one you want to use for that user by running ``export LANG=en_US.UTF-8`` (or whichever scheme you want to use). You should not need to edit any source code, as has been a popular suggestion over the past few years.

Unicode Emoji Support
~~~~~~~~~~~~~~~~~~~~~
Another issue encountered when using Eggdrop is that Unicode Emojis are not supported- this is not Eggdrop specific; rather it is a "feature" of Tcl. Unfortunately, the only solution at this time to enable Emojis (and other high-number characters) is to recompile with the TCL_UTF_MAX=6 compile flag. At the time of writing, we are unaware of a package-manager version of this that would remedy the problem. To understand more on the 'why' behind this, you can read this `Tcl Improvement Proposal <https://core.tcl-lang.org/tips/doc/trunk/tip/389.md>`_.
