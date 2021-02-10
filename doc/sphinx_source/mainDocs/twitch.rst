######
Twitch
######

This module *attempts* to provide connectivity with the Twitch gaming platform. While Twitch provides an IRC gateway to connect with it's streaming service, it does not claim to (and certainly does not) follow the IRC RFC in any meaningful way. The intent of this module is not to provide the full spectrum of management functions typically associated with Eggdrop; instead it focuses around the following key functions:

* Logging of general and Twitch-specific events (raids, blocks, bit donations)
* Tracking userstate and roomstate values
* Adding Tcl event binds for a number of Twitch events

**********
Disclaimer
**********
We should also make clear that Eggdrop is in no way affiliated with Twitch in any way, and Twitch fully controls their own platform, to include the IRC gateway. This was just a fun project implemented at the request of some users to interact with the Twitch IRC development gateway as it existed at the time of development. At any time, Twitch could choose to alter or discontinue their IRC connectivity, thereby rendering this Eggdrop module useless. Eggdrop developers are also unable to offer technical support for Twitch-specific issues encountered while using this module.

***********************
Registering with Twitch
***********************
#. Register an account with Twitch. At the time of writing, this is done by visiting `Twitch <http://twitch.tv/>`_ and clicking on the Sign Up button.
#. Generate a token to authenticate your bot with Twitch. At the time of writing, this is done by visiting the `Twitch OAuth generator <https://twitchapps.com/tmi/>`_ while logged in to the account you just created. The token will be an alphanumeric string and should be treated like a password (...because it is). Make note of it, and keep it safe!

***********************
Editing the config file
***********************

#. Find addserver options in the server section of the config file. Remove the sample servers listed and add the following line in their place, replacing the alphanumeric string after 'oauth:' with the token you created when registering with Twitch in the previous section. Pretending your Twitch token from the previous step is 'j9irk4vs28b0obz9easys4w2ystji3u', it should look like this::

    addserver irc.chat.twitch.tv 6667 oauth:j9irk4vs28b0obz9easys4w2ystji3u

Make sure you leave the 'oauth:' there, including the ':'.

#. Spoiler alert- this step tells you to do nothing, but we're including it for the sake of thoroughness. We know you are the ideal IRC user and have read all the docs for Twitch already, and noticed that Eggdrop has to request capabilities from the Twitch via a CAP request, something that is normally set in the config file. Good news! The Twitch module does this for you already, automatically. No need to edit the ``cap-request`` setting in the config. Moving on!

#. Find the Twitch section of the config file, and enable the loading of the twitch module by removing the '#' in front of the loadmodule command. It should look like this when you are done::

    loadmodule twitch

#. Start your bot as usual, and good luck!

*************************
Twitch web UI functions
*************************

Twitch is normally accessed via a web UI, and uses commands prefixed with a . or a / to interact with the channel. The Twitch module adds the partyline command ``twcmd`` to replicate those Twitch-specific commands. For example, to grant VIP status to a user via Tcl, you would use the command ``.twcmd vip username``. or to restrict chat to subscribers, you would use ``.twcmd subscribers`` (Note: no . or / is needed as a prefix to the Twitch command). In other words, ``.twcmd`` in Tcl is the interface to the standard Twitch set of commands available through the web UI (Also available as a Tcl command).

**********************
Twitch IRC limitations
**********************
There are a few things you should know about how Twitch provides service through the IRC gateway that affects how well Eggdrop can function:

* Twitch does not broadcast JOINs or PARTs for channels over 1,000 users. This renders tracking users on a channel unreliable.
* Twitch does not broadcast MODE changes for moderator status. This (mostly) renders tracking the status of users infeasible.
* Twitch stores bans on its servers (and does not accept MODE +b), making the Eggdrop ban list (and exempts/invites) mostly useless
* Twitch does not allow clients to issue MODE +o/-o commands, preventing Eggdrop from op'ing users through the traditional method

In light of these limitations, Eggdrop developers made an intentional decision to leave some non-compatible capabilities present with the Twitch module, most notably on the Tcl side. For example, if functionality such as the Tcl topic bind were to be removed, already-written scripts that had that feature in them would simply not load, rendering the entire Tcl script useless. By leaving these capabilities in, the hope is that existing Tcl scripts can still be used with Twitch, even if in a degraded capacity. As such, do be careful with the scripts you load as you may face errors you had not encountered before.
