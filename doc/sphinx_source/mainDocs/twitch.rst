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
#. Generate a token to authenticate your bot with Twitch. At the time of writing, this is done by visiting the `Twitch OAuth generator <https://twitchapps.com/tmi/>`_ while logged in to the account you just created. The token will be an alphanumeric string and should be trated like a password (...because it is). Make note of it, and keep it safe!

***********************
Editing the config file
***********************

#. Find addserver options in the server section of the config file. Remove the sample servers listed and add the following line in their place, replacing the alphanumeric string after 'oauth:' with the token you created when registering with Twitch in the previous section. Pretending your Twitch token from the previous step is 'j9irk4vs28b0obz9easys4w2ystji3u', it should look like this::

    addserver irc.chat.twitch.tv 6667 oauth:j9irk4vs28b0obz9easys4w2ystji3u

Make sure you leave the 'oauth:' there, including the ':'.

2. Find ``set cap-request`` setting in the CAP section of the config file. We want to negotiate the ability to use Twitch-specific capabilities with the server, so edit that line so it looks like this::

    set cap-request "twitch.tv/commands twitch.tv/membership twitch.tv/tags"

#. Find the keep-nick setting below the CAP section of the config file, and set it to 0.

#. Find the Twitch section of the config file, and enable the loading of the twitch module by removing the '#' in front of the loadmodule command. It should look like this when you are done::

    loadmodule twitch

#. Start your bot as usual, and good luck!

*************************
Twitch web UI functions
*************************

Twitch is normally accessed via a web UI, and uses commands prefixed with a . or a / to interact with the channel. The Twitch module adds the Tcl command ``twcmd`` to replicate those Twitch-specific commands. For example, to grant VIP status to a user via Tcl, you would use the command ``twcmd vip username``. or to restrict chat to subscibers, you would use ``twcmd subscribers``. In other words, ``twcmd`` in Tcl is the interface to the standard Twitch set of commands available through the web UI.

*****************************************
Tracking users and status with twitch.tcl
*****************************************
IMPORTANT: To use this script, you need to install additional Tcl libraries. On a Debian/Ubuntu-based system, you can do this by running::

    sudo apt-get install tcllib tcl-tls

Then, configure the section below and load this script by adding::
    source scripts/twitch.tcl

at the bottom of your Eggdrop's config file.

Finally, to enable this tracking for a channel, join the partyline of your eggdrop and enable the functionality with::

    .chanset #examplechan +twitchusers
 
As mentioned in the `Disclaimer`_, the API used for this is developed and maintained by Twitch (not Eggdrop!) but is undocumented and not official. As such, we want to be good stewards of this functionality and not be part of the reason it could be taken away, which hundreds of Eggdrops hammering it every minute could do. To that end, we built this script to require manual refreshes of the data as opposed to a scheduled polling of the API. Eggdrop will load this data once when it joins a channel (if you enabled the +twitchusers channel flag via ``.chanset`` on the partyline), and only again if you call ``twitch:updateusers`` or one of the is* commands with the update flag specified. In other words- this list of users is should only be considered current immediately after you call it yourself; it will not update on its own.

To get create/update a list of users and their statuses on a channel, use the ``twitch:updateusers [#channel]`` command. This will load the ``$tw`` array with the values obtained from the web API in a dict format. To use these values in a script, use ``$tw(#channel)`` to view all the users and statuses, and the dict command to grab specific values. For example::

    twitch:updateusers #examplechan
    > Updating Twitch user statuses for #examplechan...
    putlog $tw(#examplechan)
    > _links {} chatter_count 2 chatters {broadcaster examplechan vips {} moderators {harry sally} staff {} admins {} global_mods {} viewers spot}
    putlog [dict get $tw(#examplechan) chatters moderators]
    > harry sally

Note that the $tw array returns a nested dict, so you need multiple args to drill down to the value you want, in this case chatters then moderators.

**********************
Twitch IRC limitations
**********************
There are a few things you should know about how Twitch provides service through the IRC gateway that affects how well Eggdrop can function:

* Twitch does not broadcast JOINs or PARTs for channels over 1,000 users. This renders tracking users on a channel unreliable.
* Twitch does not broadcast MODE changes for moderator status. This (mostly) renders tracking the status of users infeasible.
* Twitch stores bans on its servers (and does not accept MODE +b), making the Eggdrop ban list (and exempts/invites) mostly useless
* Twitch does not allow clients to issue MODE +o/-o commands, preventing Eggdrop from op'ing users through the traditional method

In light of these limitations, Eggdrop developers made an intentional decision to leave some non-compatible capabilities present with the Twitch module, most notably on the Tcl side. For example, if functionality such as the Tcl topic bind were to be removed, already-written scripts that had that feature in them would simply not load, rendering the entire Tcl script useless. By leaving these capabilities in, the hope is that existing Tcl scripts can still be used with Twitch, even if in a degraded capacity. As such, do be careful with the scripts you load as you may face errors you had not encountered before.
