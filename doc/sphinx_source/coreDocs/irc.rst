Last revised: August 21, 2004

.. _irc:

==========
IRC Module
==========

This module controls the bots interaction on IRC. It allows the bot to
join channels, etc. You have to load this if you want your bot to come on
irc.

  This module requires: server, channels

Put this line into your Eggdrop configuration file to load the irc
module::

  loadmodule irc

There are also some variables you can set in your config file:

  set bounce-bans 1
    Set this to 1 if you want to bounce all server bans.

  set bounce-exempts 0
    Set this to 1 if you want to bounce all server exemptions (+e modes).
    This is disabled if use-exempts is disabled.

  set bounce-invites 0
    Set this to 1 if you want to bounce all server invitations (+I modes).
    This is disabled if use-invites is disabled.

  set bounce-modes 0
    Set this to 1 if you want to bounce all server modes.

  set max-modes 30
    There is a global limit for +b/+e/+I modes. This limit should be set to
    the same value as max-bans for networks that do not support +e/+I.

  set max-bans 30
    Set here the maximum number of bans you want the bot to set on a channel.
    Eggdrop will not place any more bans if this limit is reached. Undernet
    currently allows 45 bans, IRCnet allows 30, EFnet allows 100, and DALnet
    allows 100.

  set max-exempts 20
    Set here the maximum number of exempts you want Eggdrop to set on a
    channel. Eggdrop will not place any more exempts if this limit is
    reached.

  set max-invites 20
    Set here the maximum number of invites you want Eggdrop to set on a
    channel. Eggdrop will not place any more invites if this limit is
    reached.

  | set use-exempts 0
  | set use-invites 0

    These settings should be left commented unless the default values are
    being overridden. By default, exempts and invites are on for EFnet and
    IRCnet, but off for all other large networks. This behavior can be
    modified with the following 2 flags. If your network doesn't support
    +e/+I modes then you will be unable to use these features.

  set learn-users 0
    If you want people to be able to add themselves to the bot's userlist
    with the default userflags (defined above in the config file) via the
    'hello' msg command, set this to 1.

  set wait-split 600
    Set here the time (in seconds) to wait for someone to return from a
    netsplit (i.e. wasop will expire afterwards). Set this to 1500 on IRCnet
    since its nick delay stops after 30 minutes.

  set wait-info 180
    Set here the time (in seconds) that someone must have been off-channel
    before re-displaying their info line.

  set mode-buf-length 200
    Set this to the maximum number of bytes to send in the arguments of
    modes sent to the server. Most servers default this to 200.

  | unbind msg - hello \*msg:hello
  | bind msg - myword \*msg:hello

    Many IRCops find bots by seeing if they reply to 'hello' in a msg. You
    can change this to another word by un-commenting thse two lines and
    changing "myword" to the word wish to use instead of'hello'. It must be
    a single word.


  | unbind msg - ident \*msg:ident
  | unbind msg - addhost \*msg:addhost
    Many takeover attempts occur due to lame users blindly /msg ident'ing to

    the bot and attempting to guess passwords. We now unbind this command by
    default to discourage them. You can enable this command by un-commenting
    these two lines.

  | set opchars "@"
  | #set opchars "@&~"

    Some IRC servers are using some non-standard op-like channel
    prefixes/modes. Define them here so the bot can recognize them. Just
    "@" should be fine for most networks. Un-comment the second line for
    some UnrealIRCds.

  set no-chanrec-info 0
    If you are so lame you want the bot to display peoples info lines, even
    when you are too lazy to add their chanrecs to a channel, set this to 1.
    *NOTE* This means *every* user with an info line will have their info
    line displayed on EVERY channel they join (provided they have been gone
    longer than wait-info).

These were the core irc module settings. There are more settings for
'net-type' 1 and 5. net-type has to be set in the server module config
section.

Use the following settings only if you set 'net-type' to 1!

  set prevent-mixing 1
    At the moment, the current IRCnet IRCd version (2.10) doesn't support the
    mixing of b, o and v modes with e and I modes. This might be changed in
    the future, so use 1 at the moment for this setting.

  Use the following settings only if you set 'net-type' to 5!

  set kick-method 1
    If your network supports more users per kick command then 1, you can
    change this behavior here. Set this to the number of users to kick at
    once, or set this to 0 for all at once.

  set modes-per-line 3
    Some networks allow you to stack lots of channel modes into one line.
    They're all guaranteed to support at least 3, so that's the default.
    If you know your network supports more, you may want to adjust this.
    This setting is limited to 6, although if you want to use a higher
    value, you can modify this by changing the value of MODES_PER_LINE_MAX
    in src/chan.h and recompiling the bot.

  set include-lk 1
    Some networks don't include the +l limit and +k or -k key modes in the
    modes-per-line (see above) limitation. Set include-lk to 0 for these
    networks.

  set use-354 0
    Set this to 1 if your network uses IRCu2.10.01 specific /who requests.
    Eggdrop can, therefore, ask only for exactly what's needed.

  set rfc-compliant 1
    If your network doesn't use rfc 1459 compliant string matching routines,
    set this to 0.

Copyright (C) 2000 - 2019 Eggheads Development Team
