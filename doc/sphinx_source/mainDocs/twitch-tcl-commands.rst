Eggdrop Twitch Tcl Commands
Last revised: April 25, 2020

===========================
Eggdrop Twitch Tcl Commands
===========================

This is an exhaustive list of all the Twitch-specific Tcl commands added to
Eggdrop. These commands are held separate from the traditional tcl-commands.doc
due to the unique nature of Twitch and the fact that these commands WILL NOT
work on a normal IRC server. Again, if you try and use these commands on a
normal IRC server they will, at best, not work and possibly have unintended
consequences.

This list is accurate for Eggdrop v1.9.0, the minimum version for which Twitch
functionality is possible.

Commands
--------

^^^^^^^^^^^^^^^^^^^^^^^^
twcmd <chan> <cmd> [arg]
^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends cmd to the server, prefixed with a '/'. This is replicates
  the web UI functionality of sending commands such as /vip or /subscribers. Do
  not include a '/' in the cmd. arg, if required, is a single argument, so
  enclose multiple strings in quotes. Example: twcmd timeout "badguy 100"

  Returns: 0 if command is successfully sent to the Twitch server, 1 on failure

^^^^^^^^^^^^^^^^^^^
userstate <channel>
^^^^^^^^^^^^^^^^^^^

  Description: provides current userstate for the Eggdrop on the given channel.

  Returns: a dict containing key/value pairs for userstate values.

^^^^^^^^^^^^^^^^^^^
roomstate <channel>
^^^^^^^^^^^^^^^^^^^

  Description: provides current roomstate of a channel Eggdrop is on.

  Returns: a dict containing key/value pairs for roomstate values.

^^^^^^^^^^^^^^^^^^^^
twitchmods <channel>
^^^^^^^^^^^^^^^^^^^^

  Description: maintains a list of usernames provided by Twitch as those that have moderator status on the provided channel. This list is refreshed upon join, or can manually be refreshed by using the Tcl ``twcmd`` to issue a /mods Twitch command. This list is a comprehensive list, the user does not need to be present on the channel to be included on this list.

  Returns: A list of usernames designated as having moderator status by Twitch.

^^^^^^^^^^^^^^^^^^^^
twitchvips <channel>
^^^^^^^^^^^^^^^^^^^^

  Description: maintains a list of usernames provided by Twitch as those that have VIP status on the provided channel. This list is refreshed upon join, or can manually be refreshed by using the Tcl ``twcmd`` to issue a /vips Twitch command. This list is a comprehensive list, the user does not need to be present on the channel to be included on this list.

  Returns: A list of usernames designated as having VIP status by Twitch.

^^^^^^^^^^^^^^^^^^^^^^
ismod <nick> [channel]
^^^^^^^^^^^^^^^^^^^^^^

  Description: checks if a user is on the moderator list maintained by Twitch (the same list accessible by the /mods command entered via the Twith web GUI). This differs from the other "normal" is* Eggdrop Tcl cmds, as this does NOT check if the user is currently on the channel (that status is unreliable on Twitch IRC).

  Returns: 1 if someone by the specified nickname is on the moderator list for the channel (or any channel if no channel name is specified); 0 otherwise.

^^^^^^^^^^^^^^^^^^^^^^
isvip <nick> [channel]
^^^^^^^^^^^^^^^^^^^^^^
  
  Description: checks if a user is on the VIP list maintained by Twitch (the same list accessible by the /vips command entered via the Twith web GUI). This differs from the other "normal" is* Eggdrop Tcl cmds, as this does NOT check if the user is currently on the channel (that status is unreliable on Twitch IRC).

  Returns: 1 if someone by the specified nickname is on the VIP list for the channel (or any channel if no channel name is specified); 0 otherwise.


Binds
-----

You can use the 'bind' command to attach Tcl procedures to certain events. The
binds listed here are in addition to the binds listed in tcl-commands.doc.

Because Twitch offers an IRC gateway that significantly reduces traditional IRC
functionality, the following binds listed in tcl-commands.doc are not
compatible with Twitch. If they are in an existing script you are running that
is fine, they just likely won't do anything.

To remove a bind, use the 'unbind' command. For example, to remove the
bind for the "stop" msg command, use 'unbind msg - stop msg:stop'.

^^^^^
Flags
^^^^^
Most of the following binds have "flags" listed as an argument for the bind. Flags represents a flagmask that the user, if found, must match in order for the bind to trigger. Example flag masks are:

+-------+---------------------------------------------------------------------------------------+
| \-    | Matches any user existing in Eggdrop's internal user list (added to the bot)          |
+-------+---------------------------------------------------------------------------------------+
| \*    | Matches any user, doesn't need to be added to Eggdrop                                 |
+-------+---------------------------------------------------------------------------------------+
| +o    | Matches a user with the global 'o' flag added to their handle on the Eggdrop          |
+-------+---------------------------------------------------------------------------------------+
| \|+f  | Matches a user with the 'f' flag added on a channel to their handle on the Eggdrop    |
+-------+---------------------------------------------------------------------------------------+
| +m|+m | Matches a user with the 'm' flag added either globally or on a channel on the Eggdrop |
+-------+---------------------------------------------------------------------------------------+

^^^^^^^^^^
Bind Types
^^^^^^^^^^

The following is a list of bind types and how they work. Below each bind type is the format of the bind command, the list of arguments sent to the Tcl proc, and an explanation.

#. CCHT  (CLEARCHAT)

  bind ccht <flags> <mask> <proc>

  procname <nick> <chan>

  Description: Called when the chat history for the channel or a user on the channel is cleared. The value of ``mask`` specified in the bind is matched against ``#channel nick!nick@nick.tmi.twitch.tv`` and can use wildcards. If a user is found that matches ``mask``, they must also match the ``flags`` mask. If the CLEARCHAT is only targeted at a specific user and not the channel, that user's nickname will appear in ``nick``, otherwise ``nick`` is empty. ``chan`` will contain the channel the CLEARCHAT was used on.

#. CMSG (CLEARMSG)

  bind cmsg <flags> <command> <proc>

  procname <nick> <chan> <msgid> <msg>

  Description: Called when a specific message on the channel is cleared. The value of ``mask`` specified in the bind is matched against ``#channel nick!nick@nick.tmi.twitch.tv`` and can use wildcards. If a user is found that matches ``mask``, they must also match the ``flags`` mask. ``nick`` contains the user's nickname, and ``chan`` will contain the channel the CLEARMSG was used on.

#. HTGT (HOSTTARGET)

  bind htgt <flags> <mask> <proc>

  procname <target> <chan> <viewers>

  Description: Called when a broadcaster starts or stops hosting another Twitch channel. ``mask`` is in the format "#channel target", where #channel is the hosting channel and target is the name of the broadcaster being hosted by #channel. Similarly for the proc, ``target`` is the name of the Twitch channel being hosted by ``chan``. A value of ``-`` in ``target`` indicates that the broadcaster has stopped hosting another channel. ``viewers`` contains the number of viewers in ``chan`` that are now watching ``target`` when hosting starts, but has been found to not be reliably provided by Twitch (often arbitrarily set to 0).

#. WSPR (WHISPER)

  bind wspr <flags> <commmand> <proc>

  procname <nick> <userhost> <handle> <msg>

  Description: Called when Eggdrop received a whisper from another Twitch user. The first word of the user's msg is matched against ``command``, and the remainder of the text is passed to ``msg``. ``nick`` is populated with the login name of the user messaging the Eggdrop, ``userhost`` contains nick's userhost in the format nick!nick@nick.tmi.twitch.tv. ``handle`` will match the user's handle on the bot if present, otherwise it will return a ``*``.

#. WSPM (WHISPER)

  bind wspr <flags> <mask> <proc>

  procname <nick> <userhost> <handle> <msg>

  Description: Called when Eggdrop received a whisper from another Twitch user. The msg is matched against ``mask``, which can contain wildcards. ``nick`` is populated with the login name of the user messaging the Eggdrop, ``userhost`` contains nick's userhost in the format nick!nick@nick.tmi.twitch.tv. ``handle`` will match the user's handle on the bot if present, otherwise it will return a ``*``. The full text of the whisper is stored in ``msg``.

#. RMST (ROOMSTATE)

  bind rmst <flags> <mask> <proc>

  procname <chan> <tags>

  Description: Called when Eggdrop receives a ROOMSTATE message. ``mask`` is in the format of ``#channel keys`` and can use wildcards. For example, to trigger this bind on #eggdrop for any change, you would use ``#eggdroptest *`` as the mask, or to trigger on #eggdrop specifically for the emote-only setting, you would use ``"#eggdrop *emote-only*"`` as the mask. Due to the nature of multiple keys per roomstate and uncertainty of ordering, it is recommended to use multiple binds if you wish to specify multiple key values. ``chan`` is the channel Eggdrop received the ROOMSTATE message for, and ``tags`` is a list of key/value pairs provided by the ROOMSTATE message, suitable for use as a Tcl dict. ``flags`` is ignored.

#. USST (USERSTATE)

  bind usst <flags> <mask> <proc>

  procname <chan> <tags>

  Description: Called when Eggdrop receives a USERSTATE message. ``mask`` is in the format of ``#channel keys`` and can use wildcards (see the RMST bind for additional details on format). ``chan`` is the channel Eggdrop received the USERSTATE message for, and ``tags`` is a list of key/value pairs provided in the USERSTATE message, suitable for use as a Tcl dict. ``flags`` is ignored.
