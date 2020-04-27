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

^^^^^^^^^^
Bind Types
^^^^^^^^^^

The following is a list of bind types and how they work. Below each bind type is the format of the bind command, the list of arguments sent to the Tcl proc, and an explanation.

#. CCHT  (CLEARCHAT)

  bind ccht <flags> <command> <proc>

  procname <nick> <chan>

  Description: Called when the chat history for the channel or a user on the channel is cleared on ``chan``. If the CLEARCHAT is only targeted at user and not the channel, that user's nickname will appear in ``nick``, otherwise ``nick`` is empty.

#. CMSG (CLEARMSG)

  bind cmsg <flags> <command> <proc>

  procname <nick> <chan> <msgid> <msg>

  Description: Called when a message is removed from a channel. The targeted user is stored in ``nick``, the channel it occured on is stored in ``chan``, the msg-id (located in the message tags) of the removed message is stored in ``msg-tag``, and the message content itself is stored in ``msg``. 

#. HTGT (HOSTTARGET)

  bind htgt <flags> 

(1)  MSG

  bind msg <flags> <command> <proc>

  procname <nick> <user\@host> <handle> <text>

  Description: used for /msg commands. The first word of the user's msg is the command, and everything else becomes the text argument.

  Module: server

