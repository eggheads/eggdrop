Last revised: October 25, 2010

=====================
Eggdrop Core Settings
=====================

This file describes the syntax and all the settings of your Eggdrop
configuration file. Please note that you don't need to set all of these
variables to make your bot work properly.

Executable Path
---------------

The first line in an Eggdrop configuration file should contain
a fully qualified path to your Eggdrop executable. It has to be
implemented in the way the example shows to make the config file
work as a shell script.

Example::

  #! /path/to/eggdrop


Basic Settings
--------------

You can change the basic Eggdrop appearance and behavior in this section.

  set username "lamest"
    This setting defines the username the bot uses on IRC. This setting has
    no effect if an ident daemon is running on your bot's machine.

  set admin "Lamer <email: lamer@lamest.lame.org>"
    This setting defines which contact person should be shown in .status,
    /msg help, and other places. You really should include this information.

  set network "SomeIrcNetwork"
    This setting is used only for info to share with others on your botnet.
    Set this to the IRC network your bot is connected to.

  set timezone "EST"
    This setting defines which timezone is your bot in. It's used for internal
    routines as well as for logfile timestamping and scripting purposes.
    The timezone string specifies the name of the timezone and must be three
    or more alphabetic characters. For example, Central European Time(UTC+1)
    should be "CET".

  set offset "5"
    The offset setting specifies the time value to be added to the local
    time to get Coordinated Universal Time (UTC aka GMT). The offset is
    positive if the local timezone is west of the Prime Meridian and
    negative if it is east. The value (in hours) must be between -23 and
    23. For example, if the timezone is UTC+1, the offset is -1.

  set env(TZ) "$timezone $offset" (disabled by default)
    If you don't want to use the timezone setting for scripting purposes
    only, but instead everywhere possible, then use this setting.

  | set vhost4 "99.99.0.0"
  | set vhost4 "virtual.host.com"
  |   If you're using virtual hosting (your machine has more than 1 IP), you
      may want to specify the particular IP to bind to. You can specify either
      by hostname or by IP. Note that this is not used for listening. Use the
      'listen-addr' variable to specify the listening address.

  | set vhost6 "2001:db8:618:5c0:263::"
  | set vhost6 "my.ipv6.host.com"
  |   IPv6 vhost to bind to for outgoing IPv6 connections. You can set it
      to any valid IPv6 address or hostname, resolving to an IPv6 address.
      Note that this is not used for listening. Use the 'listen-addr'
      variable to specify the listening address.

  | set listen-addr "99.99.0.0"
  | set listen-addr "2001:db8:618:5c0:263::"
  | set listen-addr "virtual.host.com"
  |   IPv4/IPv6 address (or hostname) to bind for listening.
      If you don't set this variable, eggdrop will listen on all available
      IPv4 or IPv6 interfaces, depending on the 'prefer-ipv6' variable.
      Note that on most platforms, IPv6 sockets are able to accept both
      IPv4 and IPv6 connections.

  set prefer-ipv6 "1"
    Prefer IPv6 over IPv4 for connections and dns resolution.
    If the preferred protocol family is not supported, other possible
    families will be tried.

  addlang "english"
    If you want to have your Eggdrop messages displayed in another language,
    change this command to match your preferences. An alternative would be
    to set the environment variable EGG_LANG to that value.

    Languages included with Eggdrop: Danish, English, French, Finnish,
    German.

Log Files
---------

Eggdrop is capable of logging various things, from channel chatter to
partyline commands and file transfers.

Logfiles are normally kept for 24 hours. Afterwards, they will be renamed
to "(logfilename).yesterday". After 48 hours, they will be overwritten by
the logfile of the next day.

  set max-logs 20
    This is the maximum number of concurrent logfiles that can be opened
    for writing at one time. At most, this value should be the maximum
    number of channels you expect to create log files for. There is no
    value for 'infinity'; very few cases should ever require more than 20.
    A decrease to this value while running will require a restart (not rehash)
    of the bot. However, don't decrease it below 5.

  set max-logsize 0
    This is the maximum size of your logfiles. Set it to 0 to disable.
    This value is in kilobytes, so '550' would mean cycle logs when it
    reaches the size of 550 kilobytes. Note that this only works if you
    have keep-all-logs set to 0 (OFF).

  set quick-logs 0
    This could be good if you have had a problem with logfiles filling
    your quota/hard disk or if you log +p and publish it to the web, and
    you need more up-to-date info. Note that this setting might increase
    the CPU usage of your bot (on the other hand it will decrease your RAM
    usage).

  set raw-log 0
    This setting allows you the logging of raw incoming server traffic via
    console/log flag 'r', raw outgoing server traffic via console/log mode
    'v', raw incoming botnet traffic via console/log mode 't', raw outgoing
    botnet traffic via console/log mode 'u', raw outgoing share traffic via
    console/log mode 'g', and raw incoming share traffic via console/log
    mode 'h'. These flags can create a large security hole, allowing people
    to see user passwords. This is now restricted to +n users only. Please
    choose your owners with care.

logfile <logflags> <channel> "logs/logfile"
    This setting tells the bot what should be logged, from where, and to
    which file.

    Logfile flags:

      +---+------------------------------------------------------+
      | b | information about bot linking and userfile sharing   |
      +---+------------------------------------------------------+
      | c | commands                                             |
      +---+------------------------------------------------------+
      | d | misc debug information                               |
      +---+------------------------------------------------------+
      | g | raw outgoing share traffic                           |
      +---+------------------------------------------------------+
      | h | raw incoming share traffic                           |
      +---+------------------------------------------------------+
      | j | joins, parts, quits, and netsplits on the channel    |
      +---+------------------------------------------------------+
      | k | kicks, bans, and mode changes on the channel         |
      +---+------------------------------------------------------+
      | l | linked bot messages                                  |
      +---+------------------------------------------------------+
      | m | private msgs, notices and ctcps to the bot           |
      +---+------------------------------------------------------+
      | o | misc info, errors, etc (IMPORTANT STUFF)             |
      +---+------------------------------------------------------+
      | p | public text on the channel                           |
      +---+------------------------------------------------------+
      | r | raw incoming server traffic                          |
      +---+------------------------------------------------------+
      | s | server connects, disconnects, and notices            |
      +---+------------------------------------------------------+
      | t | raw incoming botnet traffic                          |
      +---+------------------------------------------------------+
      | u | raw outgoing botnet traffic                          |
      +---+------------------------------------------------------+
      | v | raw outgoing server traffic                          |
      +---+------------------------------------------------------+
      | w | wallops (make sure the bot sets +w in init-server)   |
      +---+------------------------------------------------------+
      | x | file transfers and file-area commands                |
      +---+------------------------------------------------------+

    Note that modes d, h, r, t, and v can fill disk quotas quickly. There are
    also eight user-defined levels (1-8) which can be used by Tcl scripts.

    Each logfile belongs to a certain channel. Events of type 'k', 'j', and
    'p' are logged to whatever channel they happened on. Most other events
    are currently logged to every channel. You can make a logfile belong to
    all channels by assigning it to channel "\*".

    Examples::

      logfile mco * "logs/eggdrop.log"
      logfile jpk #lamest "logs/lamest.log"

    In 'eggdrop.log', put private msgs/ctcps, commands, misc info, and errors
    from any channel.

    In 'lamest.log', log all joins, parts, kicks, bans, public chatter, and
    mode changes from #lamest.

  set log-time 1
    Use this feature to timestamp entries in the log file.

  set timestamp-format "[%H:%M:%S]"
    Set the following to the timestamp for the logfile entries. Popular times
    might be "[%H:%M]" (hour, min), or "[%H:%M:%S]" (hour, min, sec).
    Read 'man strftime' for more formatting options.  Keep it below 32 chars.

  set keep-all-logs 0
    If you want to keep your logfiles forever, turn this setting on. All
    logfiles will get the suffix
    ".[day, 2 digits][month, 3 letters][year, 4 digits]". Note that your
    quota/hard-disk might be filled by this, so check your logfiles
    often and download them.

  set switch-logfiles-at 300
    You can specify when Eggdrop should switch logfiles and start fresh.
    use military time for this setting. 300 is the default, and describes
    03:00 (AM).

  set quiet-save 0
    "Writing user file..." and "Writing channel file..." messages won't be
    logged anymore if this option is enabled. If you set it to 2, the
    "Backing up user file..." and "Backing up channel file..." messages will
    also not be logged. In addition to this, you can disable the "Switching
    logfiles..." and the new date message at midnight, by setting this to 3.

  set logfile-suffix ".%d%b%Y"
    If keep-all-logs is 1, this setting will define the suffix of the
    logfiles. The default will result in a suffix like "04May2000". "%Y%m%d"
    will produce the often used yyyymmdd format. Read the strftime manpages
    for more options. NOTE: On systems which don't support strftime, the
    default format will be used _always_.

Console Settings
----------------

  set console "mkcoblxs"
    This is the default console mode. It uses the same event flags as the
    log files do. The console channel is automatically set to your "primary"
    channel, which is set in the modules section of the config file. Masters
    can change their console channel and modes with the '.console' command.

File and Directory Settings
---------------------------

  set userfile "LamestBot.user"
    Specify here the filename your userfile should be saved as.


  set pidfile "pid.LamestBot"
    Specify here the filename Eggdrop will save its pid to. If no pidfile is
    specified, pid.(botnet-nick) will be used.


  set help-path "help/"
    Specify here where Eggdrop should look for help files. Don't modify this
    setting unless you know what you're doing!


  set text-path "text/"
    Specify here where Eggdrop should look for text files. This is used for
    certain Tcl and DCC commands.


  set motd "text/motd"
    The MOTD (Message Of The day) is displayed when people dcc chat or telnet
    to the bot. Look at doc/TEXT-SUBSTITUTIONS for options.


  set telnet-banner "text/banner"
    This banner will be displayed on telnet connections. Look at
    doc/text-substitutions.doc for options.


  set userfile-perm 0600
    This specifies what permissions the user, channel, and notes files should
    be set to. The octal values are the same as for the chmod system command.

    To remind you::

      |      u  g  o           u  g  o           u  g  o
      |0600  rw-------   0400  r--------   0200  -w-------    u - user
      |0660  rw-rw----   0440  r--r-----   0220  -w--w----    g - group
      |0666  rw-rw-rw-   0444  r--r--r--   0222  -w--w--w-    o - others

    Note that the default 0600 is the most secure one and should only be
    changed if you need your files for shell scripting or other external
    applications.

  set mod-path "modules/"
    This path specifies the path were Eggdrop should look for its modules.
    If you run the bot from the compilation directory, you will want to set
    this to "". If you use 'make install' (like all good kiddies do ;), this
    is a fine default. Otherwise, use your head :).

Botnet/Dcc/Telnet Settings
--------------------------

Settings in this section should be unimportant for you until you deal with
botnets (multiple Eggdrops connected together to maximize efficiency). You
should read doc/BOTNET before modifying these settings.

  set botnet-nick "LlamaBot" (disabled by default)
    If you want to use a different nickname on the botnet than you use on
    IRC (i.e. if you're on an un-trusted botnet), un-comment this line
    and set it to the nick you would like to use.

  listen <port> <mode>
    This opens a telnet port by which you and other bots can interact with
    the Eggdrop by telneting in.

    There are more options for the listen command in doc/tcl-commands.doc.
    Note that if you are running more than one bot on the same machine, you
    will want to space the telnet ports at LEAST 5 apart, although 10 is even
    better.

    Valid ports are typically anything between 1025 and 65535 assuming the
    port is not already in use.

    If you would like the bot to listen for users and bots in separate ports,
    use the following format::

      listen 3333 bots
      listen 4444 users

    If you wish to use only one port, use this format::

      listen 3333 all

    You can setup a SSL port by prepending a plus sign to it::

      listen +5555 all

    You need to un-comment this line and change the port number in order to
    open the listen port. You should not keep this set to 3333.

  set remote-boots 2
    This setting defines whether or not people can boot users on the Eggdrop
    from other bots in your botnet. Valid settings are:

    +---+----------------------------+
    | 0 | allow *no* outside boots   |
    +---+----------------------------+
    | 1 | allow boots from sharebots |
    +---+----------------------------+
    | 2 | allow any boots            |
    +---+----------------------------+

  set share-unlinks 1
    This setting prohibits Eggdrop to unlink from share bots if an remote
    bots tells so.

  set protect-telnet 0
    This setting will drop telnet connections not matching a known host.

  set dcc-sanitycheck 0
    This setting will make the bot ignore DCC chat requests which appear to
    have bogus information on the grounds that the user may have been trying
    to make the bot connect to somewhere that will get it into trouble, or
    that the user has a broken client, in which case the connect wouldn't
    work anyway.

  set ident-timeout 5
    This setting defines the time in seconds the bot should wait for ident reply
    before the lookup fails. The default ident on timeout is 'telnet'.

  set require-p 0
    Define here whether or not a +o user still needs the +p flag to dcc the
    bot.

  set open-telnets 0
    If you want people allow to telnet in and type 'NEW' to become a new user,
    set this to 1. This is similar to the 'hello' msg command. The
    protect-telnet setting must be set to 0 to use this.

  set stealth-telnets 0
    If you don't want Eggdrop to identify itself as an Eggdrop on a telnet
    connection, set this setting to 1. Eggdrop will display 'Nickname'
    instead.

  set use-telnet-banner 0
    If you want Eggdrop to display a banner when telneting in, set this
    setting to 1. The telnet banner is set by 'set telnet-banner'.

  set connect-timeout 15
    This setting defines a time in seconds that the bot should wait before
    a dcc chat, telnet, or relay connection times out.

  set dcc-flood-thr 3
    Specify here the number of lines to accept from a user on the partyline
    within 1 second before they are considered to be flooding and therefore
    get booted.

  set telnet-flood 5:60
    Define here how many telnet connection attempts in how many seconds from
    the same host constitute a flood. The correct format is Attempts:Seconds.

  set paranoid-telnet-flood 1
    If you want telnet-flood to apply even to +f users, set this setting
    to 1.

  set resolve-timeout 15
    Set here the amount of seconds before giving up on hostname/address
    lookup (you might want to increase this if you are on a slow network).

Advanced Settings
-----------------

  set firewall "!sun-barr.ebay:3666"
    Set this to your socks host if your Eggdrop sits behind a firewall. If
    you use a Sun "telnet passthru" firewall, prefix the host with a "!".

  set nat-ip "127.0.0.1"
    If you have a NAT firewall (you box has an IP in one of the following
    ranges: 192.168.0.0-192.168.255.255, 172.16.0.0-172.31.255.255,
    10.0.0.0-10.255.255.255 and your firewall transparently changes your
    address to a unique address for your box) or you have IP masquerading
    between you and the rest of the world, and /dcc chat, /ctcp chat or
    userfile sharing aren't working, enter your outside IP here. This IP
    is used for transfers only, and has nothing to do with the vhost4/6 or
    listen-addr settings. You may still need to set them.

  set reserved-portrange 2010:2020
    If you want all dcc file transfers to use a particular portrange either
    because you're behind a firewall, or for other security reasons, set it
    here.

  set ignore-time 15
    Set the time in minutes that temporary ignores should last.

  set hourly-updates 00
    Define here what Eggdrop considers 'hourly'. All calls to it, including
    such things as note notifying or userfile saving, are affected by this.

    Example::

      set hourly-updates 15

    The bot will save its userfile 15 minutes past every hour.

  set owner "MrLame, MrsLame"
    Un-comment this line and set the list of owners of the bot.
    You NEED to change this setting.

  set notify-newusers "$owner"
    Who should a note be sent to when new users are learned?

  set default-flags "hp"
    Enter the flags that all new users should get by default. See '.help
    whois' on the partyline for a list of flags and their descriptions.

  set whois-fields "url birthday"
    Enter all user-defined fields that should be displayed in a '.whois'.
    This will only be shown if the user has one of these extra fields.
    You might prefer to comment this out and use the userinfo1.0.tcl script
    which provides commands for changing all of these.

  | unbind dcc n tcl \*dcc:tcl
  | unbind dcc n set \*dcc:set
  |   Comment these two lines if you wish to enable the .tcl and .set commands.
      If you select your owners wisely, you should be okay enabling these.

  set must-be-owner 1
    If you enable this setting, only permanent owners (owner setting) will
    be able to use .tcl and .set. Moreover, if you want to only let permanent
    owners use .dump, then set this to 2.

  unbind dcc n simul \*dcc:simul
    Comment out this line to add the 'simul' partyline command (owners
    can manipulate other people on the party line). Please select owners
    wisely and use this command ethically!

  set max-dcc 50
    Set here the maximum number of dcc connections you will allow. You can
    increase this later, but never decrease it.

  set allow-dk-cmds 1
    Enable this setting if you want +d & +k users to use commands bound
    as -\|-.

  set dupwait-timeout 5
    If your Eggdrop rejects bots that actually have already disconnected
    from another hub, but the disconnect information has not yet spread
    over the botnet due to lag, use this setting. The bot will wait
    dupwait-timeout seconds before it checks again and then finally
    reject the bot.

  set strict-host 1
    Set this to 0 if you want the bot to strip '~+-^=' characters from
    user@hosts before matching them. This setting is currently kept for
    compatibility, but will be removed from the next release. Please leave
    it set to 1 for now to avoid problems with your user files in the future.

  set cidr-support 0
    Enables cidr support for b/e/I modes if set to 1. This means the bot
    will understand and match modes in cidr notation, and will be able to
    put and enforce such bans or unban itself, if banned with a cidr mask.
    Do NOT set this, if your network/server does not support cidr!

SSL Settings
------------

Settings in this section take effect when eggdrop is compiled with TLS
support.

  set ssl-privatekey "eggdrop.key"
    File containing your private key, needed for the SSL certificate
    (see below). You can create one issuing the following command::

      openssl genrsa -out eggdrop.key 4096

    It will create a 4096 bit RSA key, strong enough for eggdrop.
    This is required for SSL hubs/listen ports, secure file transfer and
    /ctcp botnick schat
    For your convenience, you can type 'make sslcert' after 'make install'
    and you'll get a key and a certificate in your DEST directory.

  set ssl-certificate "eggdrop.crt"
    Specify the filename where your SSL certificate is located. If you
    don't set this, eggdrop will not be able to act as a server in SSL
    connections, as with most ciphers a certificate and a private key
    are required on the server side. Must be in PEM format.
    If you don't have one, you can create it using the following command::

      openssl req -new -key eggdrop.key -x509 -out eggdrop.crt -days 365

    This is required for SSL hubs/listen ports, secure file transfer and
    /ctcp botnick schat
    For your convenience, you can type 'make sslcert' after 'make install'
    and you'll get a key and a certificate in your DEST directory.

  set ssl-verify-depth 9
    Sets the maximum depth for the certificate chain verification that shall
    be allowed for ssl. When certificate verification is enabled, any chain
    exceeding this depth will fail verification.

  | set ssl-capath "/etc/ssl/"
  | set ssl-cafile ""
  |   Specify the location at which CA certificates for verification purposes
      are located. These certificates are trusted. If you don't set this,
      certificate verification will not work.


  set ssl-ciphers ""
    Specify the list of ciphers (in order of preference) allowed for use with
    ssl. The cipher list is one or more cipher strings separated by colons,
    commas or spaces. Unavailable ciphers are silently ignored unless no
    usable cipher could be found. For the list of possible cipher strings
    and their meanings, please refer to the ciphers(1) manual.
    Note: if you set this, the value replaces any ciphers OpenSSL might use by
    default. To include the default ciphers, you can put DEFAULT as a cipher
    string in the list.
    For example::

      set ssl-ciphers "DEFAULT ADH"

    ... will make eggdrop allow the default OpenSSL selection plus anonymous
    DH ciphers.

    ::

      set ssl-ciphers "ALL"

    ... will make eggdrop allow all ciphers supported by OpenSSL, in a
    reasonable order.


  set ssl-cert-auth 0
    Enable certificate authorization. Set to 1 to allow users and bots to
    identify automatically by their certificate fingerprints. Setting it
    to 2 to will force fingerprint logins. With a value of 2, users without
    a fingerprint set or with a certificate UID not matching their handle
    won't be allowed to login on SSL enabled telnet ports. Fingerprints
    must be set in advance with the .fprint and .chfinger commands.
    NOTE: this setting has no effect on plain-text ports.

  You can control SSL certificate verification using the following variables.
  All of them are flag-based. You can set them by adding together the numbers
  for all exceptions you want to enable. By default certificate verification
  is disabled and all certificates are assumed to be valid.

  The options are the following:

      +---+---------------------------------------------+
      | 0 | disable verification                        |
      +---+---------------------------------------------+
      | 1 | enable certificate verification             |
      +---+---------------------------------------------+
      | 2 | allow self-signed certificates              |
      +---+---------------------------------------------+
      | 4 | don't check peer common or alt names        |
      +---+---------------------------------------------+
      | 8 | allow expired certificates                  |
      +---+---------------------------------------------+
      | 16| allow certificates which are not valid yet  |
      +---+---------------------------------------------+
      | 32| allow revoked certificates                  |
      +---+---------------------------------------------+

  set ssl-verify-dcc 0
    Control certificate verification for DCC chats (only /dcc chat botnick)

  set ssl-verify-bots 0
    Control certificate verification for linking to hubs

  set ssl-verify-clients 0
    Control cerfificate verification for SSL listening ports. This includes
    leaf bots connecting, users telneting in and /ctcp bot chat.

Modules
-------

After the core settings, you should start loading modules. Modules are
loaded by the command "loadmodule <module>". Eggdrop looks for modules
in the directory you specified by the module-path setting in the files
and directories section.

Please note that for different configurations, different modules are needed.
Four examples:

  Channel Security Bot:
    This bot needs the channels, blowfish, console, dns, irc, and (if you
    like) ctcp modules loaded. More is not needed and makes the bot slower.

  Public IRC Bot:
    A public bot should have all modules available loaded since they provide
    all functions for everyday use.

  Secure Filesys Bot:
    This bot needs all normal IRC operating modules, but not the notes, seen,
    ctcp or share modules.

  Limbo Bot:
    A limbo bot (serves as a botnet hub outside IRC) just needs the
    channels, console, dns, and maybe notes or share modules loaded. Of
    course, blowfish needs to be loaded here, too.

Scripts
-------

The scripts section should be placed at the end of the config file. All
modules should be loaded and their variables should be set at this point.

  source scripts/script.tcl
    This line loads script.tcl from the scripts directory inside your
    Eggdrop's directory. All scripts should be put there, although you can
    place them where you like as long as you can supply a fully qualified
    path to them.

    Some commonly loaded scripts are alltools.tcl and action.fix.tcl.

    The appropriate source lines are::

      source scripts/alltools.tcl
      source scripts/action.fix.tcl

Copyright (C) 2000 - 2019 Eggheads Development Team
