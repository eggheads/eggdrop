Last revised: April 20, 2020

.. _twitch:

=============
Twitch Module
=============

This module *attempts* to provide connectivity with the Twitch gaming platform. While Twitch provides an IRC gateway to connect with it's streaming service, it does not claim to (and certainly does not) follow the IRC RFC in any meaningful way. The intent of this module is not to provide the full spectrum of management functions typically associated with Eggdrop; instead it focuses around the following key functions:

* Logging of general and Twitch-specific events (raids, blocks, bit donations)
* Tracking userstate and roomstate values
* Adding Tcl event binds for a number of Twitch events

This module requires: irc.mod

Put this line into your Eggdrop configuration file to load the twitch module::

  loadmodule twitch

and set `net-type "Twitch"` in your config file.

-----------
Limitations
-----------

There are a few things you should know about how Twitch provides service through the IRC gateway that affects how well Eggdrop can function:
* Twitch does not broadcast JOINs or PARTs for channels over 1,000 users. This renders tracking users on a channel unreliable.
* Twitch does not broadcast MODE changes for moderator status. This (mostly) renders tracking the status of users infeasible. (See Tcl below section for workaround)
* Twitch stores bans on its servers (and does not accept MODE +b), making the Eggdrop ban list (and exempts/invites) mostly useless
* Twitch does not allow clients to issue MODE +o/-o commands, preventing Eggdrop from op'ing users through the traditional method

... but the good news is, we have extended much of the Twitch functionality to Tcl!

-------
Tcl API 
-------

That last section was a little bit of a downer, but don't worry, we added a TON of functionality to the Tcl API. This module adds binds for the following Twitch events:

* CLEARCHAT
* CLEARMSG
* HOSTTARGET
* WHISPER
* ROOMSTATE
* USERSTATE

------------------
Partyline commands
------------------

This module adds the following commands to the partyline:
* userstate - Lists current userstate on a channel
* roomsstate - Lists current roomstate for a channel
* twcmd - Issues a traditional Twitch web interface command to the Twitch server (/ban, /block, /host, etc)

  Copyright (C) 2020 - 2021 Eggheads Development Team

