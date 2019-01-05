Eggdrop Tcl Commands
Last revised: December 14, 2017

====================
Eggdrop Tcl Commands
====================


This is an exhaustive list of all the Tcl commands added to Eggdrop. All
of the normal Tcl built-in commands are still there, of course, but you
can also use these to manipulate features of the bot. They are listed
according to category.

This list is accurate for Eggdrop v1.8.4. Scripts written for v1.3, v1.4
or 1.6 series of Eggdrop should probably work with a few minor modifications
depending on the script. Scripts which were written for  v0.9, v1.0, v1.1
or v1.2 will probably not work without modification. Commands which have
been changed in this version of Eggdrop (or are just new commands) are
marked with vertical bars (|) on the left.

Output Commands
---------------

^^^^^^^^^^^^^^^^^^^^^^^^
putserv <text> [options]
^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends text to the server, like '.dump' (intended for direct server commands); output is queued so that the bot won't flood itself off the server.

  Options:
  -next    push messages to the front of the queue
  -normal  no effect

  Returns: nothing

  Module: server

^^^^^^^^^^^^^^^^^^^^^^^^
puthelp <text> [options]
^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends text to the server, like 'putserv', but it uses a different queue intended for sending messages to channels or people.

  Options:
  -next    push messages to the front of the queue
  -normal  no effect

  Returns: nothing

  Module: server

^^^^^^^^^^^^^^^^^^^^^^^^^
putquick <text> [options]
^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends text to the server, like 'putserv', but it uses a different (and faster) queue.

  Options:
  -next    push messages to the front of the queue
  -normal  no effect

  Returns: nothing

  Module: server

^^^^^^^^^^^^^^^^^^^^^^^^
putnow <text> [-oneline]
^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends text to the server immediately, bypassing all queues. Use with caution, as the bot may easily flood itself off the server.

  Options:
  -oneline  send text up to the first \r or \n, discarding the rest

  Returns: nothing

  Module: server

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
putkick <channel> <nick,nick,...> [reason]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends kicks to the server and tries to put as many nicks into one kick command as possible.

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^
putlog <text>
^^^^^^^^^^^^^

  Description: logs <text> to the logfile and partyline if the 'misc' flag (o) is active via the 'logfile' config file setting and the '.console' partyline setting, respectively.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^
putcmdlog <text>
^^^^^^^^^^^^^^^^

  Description: logs <text> to the logfile and partyline if the 'cmds' flag (c) is active via the 'logfile' config file setting and the '.console' partyline setting, respectively.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^
putxferlog <text>
^^^^^^^^^^^^^^^^^

  Description: logs <text> to the logfile and partyline if the 'files' flag (x) is active via the 'logfile' config file setting and the '.console' partyline setting, respectively.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
putloglev <flag(s)> <channel> <text>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: logs <text> to the logfile and partyline at the log level of the specified flag. Use "*" in lieu of a flag to indicate all log levels.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^
dumpfile <nick> <filename>
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: dumps file from the help/text directory to a user on IRC via msg (one line per msg). The user has no flags, so the flag bindings won't work within the file.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^
queuesize [queue]
^^^^^^^^^^^^^^^^^

  Returns: the number of messages in all queues. If a queue is specified, only the size of this queue is returned. Valid queues are: mode, server, help.

  Module: server

^^^^^^^^^^^^^^^^^^
clearqueue <queue>
^^^^^^^^^^^^^^^^^^

  Description: removes all messages from a queue. Valid arguments are: mode, server, help, or all.

  Returns: the number of deleted lines from the specified queue.

  Module: server

User Record Manipulation Commands
---------------------------------

^^^^^^^^^^
countusers
^^^^^^^^^^

  Returns: number of users in the bot's database

  Module: core

^^^^^^^^^^^^^^^^^^
validuser <handle>
^^^^^^^^^^^^^^^^^^

  Returns: 1 if a user by that name exists; 0 otherwise

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^
finduser <nick!user@host>
^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: finds the user record which most closely matches the given nick!user\@host

  Returns: the handle found, or "*" if none

  Module: core

^^^^^^^^^^^^^^^^
userlist [flags]
^^^^^^^^^^^^^^^^

  Returns: a list of users on the bot. You can use the flag matching system here ([global]{&/\|}[chan]{&/\|}[bot]). '&' specifies "and"; '|' specifies "or".

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^
passwdok <handle> <pass>
^^^^^^^^^^^^^^^^^^^^^^^^

  Description: checks the password given against the user's password. Check against the password "-" to find out if a user has no password set.

  Returns: 1 if the password matches for that user; 0 otherwise. Or if we are checking against the password "-": 1 if the user has no password set; 0 otherwise.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getuser <handle> [entry-type] [extra info]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: an interface to the new generic userfile support. Without an entry-type, it returns a flat key/value list (dict) of all set entries. Valid entry types are:

  +----------+-------------------------------------------------------------------------------------+
  | BOTFL    | returns the current bot-specific flags for the user (bot-only)                      |
  +----------+-------------------------------------------------------------------------------------+
  | BOTADDR  | returns a list containing the bot's address, bot listen port, and user listen port  |
  +----------+-------------------------------------------------------------------------------------+
  | HOSTS    | returns a list of hosts for the user                                                |
  +----------+-------------------------------------------------------------------------------------+
  | LASTON   | returns a list containing the unixtime last seen and the last seen place.           |
  |          | LASTON #channel returns the time last seen time for the channel or 0 if no info     |
  |          | exists.                                                                             |
  +----------+-------------------------------------------------------------------------------------+
  | INFO     | returns the user's global info line                                                 |
  +----------+-------------------------------------------------------------------------------------+
  | XTRA     | returns the user's XTRA info                                                        |
  +----------+-------------------------------------------------------------------------------------+
  | COMMENT  | returns the master-visible only comment for the user                                |
  +----------+-------------------------------------------------------------------------------------+
  | HANDLE   | returns the user's handle as it is saved in the userfile                            |
  +----------+-------------------------------------------------------------------------------------+
  | PASS     | returns the user's encrypted password                                               |
  +----------+-------------------------------------------------------------------------------------+

  For additional custom user fields, to include the deprecated "EMAIL" and "URL" fields, reference scripts/userinfo.tcl.

  Returns: info specific to each entry-type

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setuser <handle> <entry-type> [extra info]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: this is the counterpart of getuser. It lets you set the various values. Other then the ones listed below, the entry-types are the same as getuser's.

  +---------+---------------------------------------------------------------------------------------+
  | Type    | Extra Info                                                                            |
  +=========+=======================================================================================+
  | PASS    | <password>                                                                            |
  |         |   Password string (Empty value will clear the password)                               |
  +---------+---------------------------------------------------------------------------------------+
  | BOTADDR | <address> [bot listen port] [user listen port]                                        |
  |         |   Sets address, bot listen port and user listen port. If no listen ports are          |
  |         |   specified, only the bot address is updated. If only the bot listen port is          |
  |         |   specified, both the bot and user listen ports are set to the bot listen port.       |
  +---------+---------------------------------------------------------------------------------------+
  | HOSTS   | [hostmask]                                                                            |
  |         |   If no value is specified, all hosts for the user will be cleared. Otherwise, only   |
  |         |   *1* hostmask is added :P                                                            |
  +---------+---------------------------------------------------------------------------------------+
  | LASTON  | This setting has 3 forms.                                                             |
  |         |                                                                                       |
  |         | <unixtime> <place>                                                                    |
  |         |   sets global LASTON time. Standard values used by Eggdrop for <place> are partyline, |
  |         |   linked, unlinked, filearea, <#channel>, and <@remotebotname>, but can be set to     |
  |         |   anything.                                                                           |
  |         |                                                                                       |
  |         | <unixtime>                                                                            |
  |         |   sets global LASTON time (leaving the place field empty)                             |
  |         |                                                                                       |
  |         | <unixtime> <channel>                                                                  |
  |         |   sets a user's LASTON time for a channel (if it is a valid channel)                  |
  +---------+---------------------------------------------------------------------------------------+

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chhandle <old-handle> <new-handle>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: changes a user's handle

  Returns: 1 on success; 0 if the new handle is invalid or already used, or if the user can't be found

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chattr <handle> [changes [channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: changes the attributes for a user record, if you include any.
  Changes are of the form '+f', '-o', '+dk', '-o+d', etc. If changes are specified in the format of \|<changes> <channel>, the channel-specific flags for that channel are altered. You can now use the +o|-o #channel format here too.

  Returns: new flags for the user (if you made no changes, the current flags are returned). If a channel was specified, the global AND the channel-specific flags for that channel are returned in the format of globalflags|channelflags. "*" is returned if the specified user does not exist.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
botattr <handle> [changes [channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: similar to chattr except this modifies bot flags rather than normal user attributes.

  Returns: new flags for the bot (if you made no changes, the current flags are returned). If a channel was specified, the global AND the channel-specific flags for that channel are returned in the format of globalflags|channelflags. "*" is returned if the specified bot does not exist.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchattr <handle> <flags> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: checks if the flags of the specified user match the flags provided. Default matching pattern uses the | (OR) convention. For example, specifying +mn for flags will check if the user has the m OR n flag.

+------------+-----------------------------------------------------------------+
| Flag Mask  | Action                                                          |
+============+=================================================================+
| +m         + Checks if the user has the m global flag                        |
+------------+-----------------------------------------------------------------+
| +mn        | Checks if the user has the m or n global flag                   |
+------------+-----------------------------------------------------------------+
| +mn&       | Checks if the user has the m and n global flag                  |
+------------+-----------------------------------------------------------------+
| \|+o #foo  | Checks if the user has the o channel flag for #foo              |
+------------+-----------------------------------------------------------------+
| &mn #foo   | Checks if the user has the m and n channel flag for #foo        |
+------------+-----------------------------------------------------------------+
| +o|+n #foo | Checks if the user has the o global flag, or the n channel      |
|            | flag for #foo                                                   |
+------------+-----------------------------------------------------------------+
| +m&+v      | Checks if the user has the m global flag, and the v channel     |
|            | flag for #foo                                                   |
+------------+-----------------------------------------------------------------+
| -m         | Checks if the user does not have the m global flag              |
+------------+-----------------------------------------------------------------+
| \|-n #foo  | Checks if the user does not have the n channel flag for #foo    |
+------------+-----------------------------------------------------------------+
| +m|-n #foo | Checks if the user has the global m flag or does not have a     |
|            | channel n flag for #foo                                         |
+------------+-----------------------------------------------------------------+
| -n&-m #foo | Searches if the user does not have the global n flag and does   |
|            | not have the channel m flag for #foo                            |
+------------+-----------------------------------------------------------------+
| ||+b       | Searches if the user has the bot flag b                         |
+------------+-----------------------------------------------------------------+

  Returns: 1 if the specified user has the flags matching the provided mask; 0 otherwise

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^
adduser <handle> [hostmask]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: creates a new user entry with the handle and hostmask given (with no password and the default flags)

  Returns: 1 if successful; 0 if the handle already exists

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
addbot <handle> <address> [botport [userport]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Description: adds a new bot to the userlist with the handle and botaddress given (with no password and no flags). <address> format is one of:

  - ipaddress
  - ipv4address:botport/userport    [DEPRECATED]
  - [ipv6address]:botport/userport  [DEPRECATED]

NOTE 1: The []s around the ipv6address argument are literal []s, not optional arguments.
NOTE 2: In the deprecated formats, an additional botport and/or userport given as follow-on arguments are ignored.

  Returns: 1 if successful; 0 if the bot already exists or a port is invalid

  Module: core

^^^^^^^^^^^^^^^^
deluser <handle>
^^^^^^^^^^^^^^^^

  Description: attempts to erase the user record for a handle

  Returns: 1 if successful, 0 if no such user exists

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^
delhost <handle> <hostmask>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: deletes a hostmask from a user's host list

  Returns: 1 on success; 0 if the hostmask (or user) doesn't exist

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
addchanrec <handle> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds a channel record for a user

  Returns: 1 on success; 0 if the user or channel does not exist

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
delchanrec <handle> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: removes a channel record for a user. This includes all associated channel flags.

  Returns: 1 on success; 0 if the user or channel does not exist

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
haschanrec <handle> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the given handle has a chanrec for the specified channel; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getchaninfo <handle> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: info line for a specific channel (behaves just like 'getinfo')

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setchaninfo <handle> <channel> <info>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sets the info line on a specific channel for a user. If info is "none", it will be removed.

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newchanban <channel> <ban> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds a ban to the ban list of a channel; creator is given credit for the ban in the ban list. lifetime is specified in minutes. If lifetime is not specified, ban-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent ban.

  Options:
      
  +-----------+-------------------------------------------------------------------------------------+
  |sticky     | forces the ban to be always active on a channel, even with dynamicbans on           |
  +-----------+-------------------------------------------------------------------------------------+


  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newban <ban> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds a ban to the global ban list (which takes effect on all channels); creator is given credit for the ban in the ban list. lifetime is specified in minutes. If lifetime is not specified, default-ban-time (usually 120) is used. Setting the lifetime to 0 makes it a permanent ban.

  Options:

  +-----------+-------------------------------------------------------------------------------------+
  |sticky     | forces the ban to be always active on a channel, even with dynamicbans on           |
  +-----------+-------------------------------------------------------------------------------------+

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newchanexempt <channel> <exempt> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds a exempt to the exempt list of a channel; creator is given credit for the exempt in the exempt list. lifetime is specified in minutes. If lifetime is not specified, exempt-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent exempt. The exempt will not be removed until the corresponding ban has been removed. For timed bans, once the time period has expired, the exempt will not be removed until the corresponding ban has either expired or been removed.

  Options:

  +-----------+-------------------------------------------------------------------------------------+
  |sticky     | forces the exempt to be always active on a channel, even with dynamicexempts on     |
  +-----------+-------------------------------------------------------------------------------------+

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newexempt <exempt> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds a exempt to the global exempt list (which takes effect on all channels); creator is given credit for the exempt in the exempt list. lifetime is specified in minutes. If lifetime is not specified, exempt-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent exempt. The exempt will not be removed until the corresponding ban has been removed.

  Options:

  +-----------+-------------------------------------------------------------------------------------+
  |sticky     | forces the exempt to be always active on a channel, even with dynamicexempts on     |
  +-----------+-------------------------------------------------------------------------------------+

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newchaninvite <channel> <invite> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds a invite to the invite list of a channel; creator is given credit for the invite in the invite list. lifetime is specified in minutes. If lifetime is not specified, invite-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent invite. The invite will not be removed until the channel has gone -i.

  Options:

  +-----------+-------------------------------------------------------------------------------------+
  |sticky     | forces the invite to be always active on a channel, even with dynamicinvites on     |
  +-----------+-------------------------------------------------------------------------------------+

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newinvite <invite> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds a invite to the global invite list (which takes effect on all channels); creator is given credit for the invite in the invite list. lifetime is specified in minutes. If lifetime is not specified, invite-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent invite. The invite will not be removed until the channel has gone -i.

  Options:

  +-----------+-------------------------------------------------------------------------------------+
  |sticky     | forces the invite to be always active on a channel, even with dynamicinvites on     |
  +-----------+-------------------------------------------------------------------------------------+

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
stickban <banmask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: makes a ban sticky, or, if a channel is specified, then it is set sticky on that channel only.

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
unstickban <banmask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: makes a ban no longer sticky, or, if a channel is specified, then it is unstuck on that channel only.

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
stickexempt <exemptmask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: makes an exempt sticky, or, if a channel is specified, then it is set sticky on that channel only.

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
unstickexempt <exemptmask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: makes an exempt no longer sticky, or, if a channel is specified, then it is unstuck on that channel only.

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
stickinvite <invitemask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Description: makes an invite sticky, or, if a channel is specified, then it is set sticky on that channel only.

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
unstickinvite <invitemask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: makes an invite no longer sticky, or, if a channel is specified, then it is unstuck on that channel only.

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^
killchanban <channel> <ban>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: removes a ban from the ban list for a channel

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^
killban <ban>
^^^^^^^^^^^^^

  Description: removes a ban from the global ban list

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
killchanexempt <channel> <exempt>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: removes an exempt from the exempt list for a channel

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^
killexempt <exempt>
^^^^^^^^^^^^^^^^^^^

  Description: removes an exempt from the global exempt list

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
killchaninvite <channel> <invite>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: removes an invite from the invite list for a channel

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^
killinvite <invite>
^^^^^^^^^^^^^^^^^^^

  Description: removes an invite from the global invite list

  Returns: 1 on success; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^
ischanjuped <channel>
^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the channel is juped, and the bot is unable to join; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^
isban <ban> [channel]
^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified ban is in the global ban list; 0 otherwise. If a channel is specified, that channel's ban list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^
ispermban <ban> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified ban is in the global ban list AND is marked as permanent; 0 otherwise. If a channel is specified, that channel's ban list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^
isexempt <exempt> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified exempt is in the global exempt list; 0 otherwise. If a channel is specified, that channel's exempt list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ispermexempt <exempt> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified exempt is in the global exempt list AND is marked as permanent; 0 otherwise. If a channel is specified, that channel's exempt list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^
isinvite <invite> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified invite is in the global invite list; 0 otherwise. If a channel is specified, that channel's invite list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isperminvite <invite> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified invite is in the global invite list AND is marked as permanent; 0 otherwise. If a channel is specified, that channel's invite list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^
isbansticky <ban> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified ban is marked as sticky in the global ban list; 0 otherwise. If a channel is specified, that channel's ban list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isexemptsticky <exempt> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified exempt is marked as sticky in the global exempt list; 0 otherwise. If a channel is specified, that channel's exempt list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isinvitesticky <invite> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified invite is marked as sticky in the global invite list; 0 otherwise. If a channel is specified, that channel's invite list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchban <nick!user@host> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified nick!user\@host matches a ban in the global ban list; 0 otherwise. If a channel is specified, that channel's ban list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchexempt <nick!user@host> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified nick!user\@host matches an exempt in the global exempt list; 0 otherwise. If a channel is specified, that channel's exempt list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchinvite <nick!user@host> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified nick!user\@host matches an invite in the global invite list; 0 otherwise. If a channel is specified, that
  channel's invite list is checked as well.

  Module: channels

^^^^^^^^^^^^^^^^^
banlist [channel]
^^^^^^^^^^^^^^^^^

  Returns: a list of global bans, or, if a channel is specified, a list of channel-specific bans. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.

  Module: channels

^^^^^^^^^^^^^^^^^^^^
exemptlist [channel]
^^^^^^^^^^^^^^^^^^^^

  Returns: a list of global exempts, or, if a channel is specified, a list of channel-specific exempts. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.

  Module: channels

^^^^^^^^^^^^^^^^^^^^
invitelist [channel]
^^^^^^^^^^^^^^^^^^^^

  Returns: a list of global invites, or, if a channel is specified, a list of channel-specific invites. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newignore <hostmask> <creator> <comment> [lifetime]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds an entry to the ignore list; creator is given credit for the ignore. lifetime is how many minutes until the ignore expires and is removed. If lifetime is not specified, ignore-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent ignore.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^
killignore <hostmask>
^^^^^^^^^^^^^^^^^^^^^
  Description: removes an entry from the ignore list

  Returns: 1 if successful; 0 otherwise

  Module: core

^^^^^^^^^^
ignorelist
^^^^^^^^^^

  Returns: a list of ignores. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, and creator. The timestamps are in unixtime format.

  Module: core

^^^^^^^^^^^^^^^^^^^
isignore <hostmask>
^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the ignore is in the list; 0 otherwise

  Module: core

^^^^
save
^^^^

  Description: writes the user and channel files to disk

  Returns: nothing

  Module: core

^^^^^^
reload
^^^^^^

  Description: loads the userfile from disk, replacing whatever is in memory

  Returns: nothing

  Module: core

^^^^^^
backup
^^^^^^
  Description: makes a simple backup of the userfile that's on disk. If the channels module is loaded, this also makes a simple backup of the channel file.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^
getting-users
^^^^^^^^^^^^^

  Returns: 1 if the bot is currently downloading a userfile from a sharebot (and hence, user records are about to drastically change); 0 if not

  Module: core

Channel Commands
----------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
channel add <name> [option-list]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: adds a channel record for the bot to monitor. The full list of possible options are given in doc/settings/mod.channels. Note that the channel options must be in a list (enclosed in {}).

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
channel set <name> <options...>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sets options for the channel specified. The full list of possible options are given in doc/settings/mod.channels.

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^
channel info <name>
^^^^^^^^^^^^^^^^^^^

  Returns: a list of info about the specified channel's settings.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
channel get <name> [setting]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: The value of the setting you specify. For flags, a value of 0 means it is disabled (-), and non-zero means enabled (+). If no setting is specified, a flat list of all available settings and their values will be returned.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^
channel remove <name>
^^^^^^^^^^^^^^^^^^^^^

  Description: removes a channel record from the bot and makes the bot no longer monitor the channel

  Returns: nothing

  Module: channels

^^^^^^^^^^^^
savechannels
^^^^^^^^^^^^

  Description: saves the channel settings to the channel-file if one is defined.

  Returns: nothing

  Module: channels

^^^^^^^^^^^^
loadchannels
^^^^^^^^^^^^
  Description: reloads the channel settings from the channel-file if one is defined.

  Returns: nothing

  Module: channels

^^^^^^^^
channels
^^^^^^^^

  Returns: a list of the channels the bot has a channel record for

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
channame2dname <channel-name>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chandname2name <channel-dname>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: these two functions are important to correctly support !channels. The bot differentiates between channel description names (chan dnames) and real channel names (chan names). The chan dnames are what you would normally call the channel, such as "!channel". The chan names are what the IRC server uses to identify the channel. They consist of the chan dname prefixed with an ID; such as "!ABCDEchannel".

  For bot functions like isop, isvoice, etc. you need to know the chan dnames. If you communicate with the server, you usually get the chan name, though. That's what you need the channame2dname function for.

  If you only have the chan dname and want to directly send raw server commands, use the chandname2name command.

  NOTE: For non-!channels, chan dname and chan name are the same.

  Module: irc

^^^^^^^^^^^^^^^^
isbotnick <nick>
^^^^^^^^^^^^^^^^

  Returns: 1 if the nick matches the botnick; 0 otherwise

  Module: server

^^^^^^^^^^^^^^^^^
botisop [channel]
^^^^^^^^^^^^^^^^^

  Returns: 1 if the bot has ops on the specified channel (or any channel if no channel is specified); 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^
botishalfop [channel]
^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the bot has halfops on the specified channel (or any channel if no channel is specified); 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^
botisvoice [channel]
^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the bot has a voice on the specified channel (or any channel if no channel is specified); 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^
botonchan [channel]
^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the bot is on the specified channel (or any channel if no channel is specified); 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^
isop <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if someone by the specified nickname is on the channel (or any channel if no channel name is specified) and has ops; 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ishalfop <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if someone by the specified nickname is on the channel (or any channel if no channel name is specified) and has halfops; 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^
wasop <nickname> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if someone that just got opped/deopped in the chan had op before the modechange; 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
washalfop <nickname> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if someone that just got halfopped/dehalfopped in the chan had halfop before the modechange; 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isvoice <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if someone by that nickname is on the channel (or any channel if no channel is specified) and has voice (+v); 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^
onchan <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Returns: 1 if someone by that nickname is on the specified channel (or any channel if none is specified); 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
nick2hand <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: the handle of a nickname on a channel. If a channel is not specified, the bot will check all of its channels. If the nick is not found, "" is returned. If the nick is found but does not have a handle, "*" is returned.

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
hand2nick <handle> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: nickname of the first person on the specified channel (if one is specified) whose nick!user\@host matches the given handle; "" is returned if no match is found. If no channel is specified, all channels are checked.

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
handonchan <handle> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the the nick!user\@host for someone on the channel (or any channel if no channel name is specified) matches for the handle given; 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^
ischanban <ban> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified ban is on the given channel's ban list (not the bot's banlist for the channel)

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ischanexempt <exempt> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified exempt is on the given channel's exempt list (not the bot's exemptlist for the channel)

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ischaninvite <invite> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the specified invite is on the given channel's invite list (not the bot's invitelist for the channel)

  Module: irc

^^^^^^^^^^^^^^^^^^
chanbans <channel>
^^^^^^^^^^^^^^^^^^

  Returns: a list of the current bans on the channel. Each element is a sublist of the form {<ban> <bywho> <age>}. age is seconds from the bot's point of view

  Module: irc

^^^^^^^^^^^^^^^^^^^^^
chanexempts <channel>
^^^^^^^^^^^^^^^^^^^^^

  Returns: a list of the current exempts on the channel. Each element is a sublist of the form {<exempts> <bywho> <age>}. age is seconds from the bot's point of view

  Module: irc

^^^^^^^^^^^^^^^^^^^^^
chaninvites <channel>
^^^^^^^^^^^^^^^^^^^^^

  Returns: a list of the current invites on the channel. Each element is a sublist of the form {<invites> <bywho> <age>}. age is seconds from the bot's point of view

  Module: irc

^^^^^^^^^^^^^^^^^^^
resetbans <channel>
^^^^^^^^^^^^^^^^^^^

  Description: removes all bans on the channel that aren't in the bot's ban list and refreshes any bans that should be on the channel but aren't

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^
resetexempts <channel>
^^^^^^^^^^^^^^^^^^^^^^

  Description: removes all exempt on the channel that aren't in the bot's exempt list and refreshes any exempts that should be on the channel but aren't

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^
resetinvites <channel>
^^^^^^^^^^^^^^^^^^^^^^

  Description: removes all invites on the channel that aren't in the bot's invite list and refreshes any invites that should be on the channel but aren't

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
resetchanidle [nick] <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: resets the channel idle time for the given nick or for all nicks on the channel if no nick is specified.

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
resetchanjoin [nick] <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Description: resets the channel join time for the given nick or for all nicks on the channel if no nick is specified.

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^
resetchan <channel> [flags]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: rereads in the channel info from the server. If flags are specified, only the required information will be reset, according to the given flags. Available flags:

  +-----+---------------------------+
  | b   | reset channel bans        |
  +-----+---------------------------+
  | e   | reset channel exempts     |
  +-----+---------------------------+
  | I   | reset channel invites     |
  +-----+---------------------------+
  | m   | refresh channel modes     |
  +-----+---------------------------+
  | t   | refresh channel topic     |
  +-----+---------------------------+
  | w   | refresh memberlist        |
  +-----+---------------------------+

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getchanhost <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: user\@host of the specified nickname (the nickname is not included in the returned host). If a channel is not specified, bot will check all of its channels. If the nickname is not on the channel(s), "" is returned.

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getchanjoin <nickname> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: timestamp (unixtime format) of when the specified nickname joined the channel if available, 0 otherwise. Note that after a channel reset this information will be lost, even if previously available.

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
onchansplit <nick> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: 1 if that nick is split from the channel (or any channel if no channel is specified); 0 otherwise

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chanlist <channel> [flags][<&|>chanflags]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: flags are any global flags; the '&' or '|' denotes to look for channel specific flags, where '&' will return users having ALL chanflags and '|' returns users having ANY of the chanflags (See matchattr above for additional examples).

  Returns: Searching for flags optionally preceded with a '+' will return a list of nicknames that have all the flags listed. Searching for flags preceded with a '-' will return a list of nicknames that do not have have any of the flags (differently said, '-' will hide users that have all flags listed). If no flags are given, all of the nicknames on the channel are returned.

  Please note that if you're executing chanlist after a part or sign bind, the gone user will still be listed, so you can check for wasop, isop, etc.

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getchanidle <nickname> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: number of minutes that person has been idle; -1 if the specified user isn't on the channel

  Module: irc

^^^^^^^^^^^^^^^^^^^^^
getchanmode <channel>
^^^^^^^^^^^^^^^^^^^^^

  Returns: string of the type "+ntik key" for the channel specified

  Module: irc

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
jump [server [[+]port [password]]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: jumps to the server specified, or (if none is specified) the next server in the bot's serverlist. If you prefix the port with a plus sign (e.g. +6697), SSL connection will be attempted.

  Returns: nothing

  Module: server

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
pushmode <channel> <mode> [arg]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends out a channel mode change (ex: pushmode #lame +o goober) through the bot's queuing system. All the mode changes will be sent out at once (combined into one line as much as possible) after the script finishes, or when 'flushmode' is called.

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^^^^^^^
flushmode <channel>
^^^^^^^^^^^^^^^^^^^

  Description: forces all previously pushed channel mode changes to be sent to the server, instead of when the script is finished (just for the channel specified)

  Returns: nothing

  Module: irc

^^^^^^^^^^^^^^^
topic <channel>
^^^^^^^^^^^^^^^

  Returns: string containing the current topic of the specified channel

  Module: irc

^^^^^^^^^^^^^^^^^^^
validchan <channel>
^^^^^^^^^^^^^^^^^^^

  Description: checks if the bot has a channel record for the specified channel. Note that this does not necessarily mean that the bot is ON the channel.

  Returns: 1 if the channel exists, 0 if not

  Module: channels

^^^^^^^^^^^^^^^^^^^
isdynamic <channel>
^^^^^^^^^^^^^^^^^^^

  Returns: 1 if the channel is a dynamic channel; 0 otherwise

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setudef <flag/int/str> <name>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: initializes a user defined channel flag, string or integer setting. You can use it like any other flag/setting. IMPORTANT: Don't forget to reinitialize your flags/settings after a restart, or it'll be lost.

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
renudef <flag/int/str> <oldname> <newname>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: renames a user defined channel flag, string, or integer setting.

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
deludef <flag/int/str> <name>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: deletes a user defined channel flag, string, or integer setting.

  Returns: nothing

  Module: channels

^^^^^^^^^^^^^^^^^^^^^^^
getudefs [flag/int/str]
^^^^^^^^^^^^^^^^^^^^^^^

  Returns: a list of user defined channel settings of the given type, or all of them if no type is given.

  Module: channels

^^^^^^^^^^^^^^^^^^^^^
chansettype <setting>
^^^^^^^^^^^^^^^^^^^^^

  Returns: The type of the setting you specify. The possible types are flag, int, str, pair. A flag type references a channel flag setting that can be set to either + or -. An int type is a channel  setting that is set to a number, such as ban-time. A str type is a  channel setting that stores a string, such as need-op. A pair type is a setting that holds a value couple, such as the flood settings.

  Module: channels

DCC Commands
------------

^^^^^^^^^^^^^^^^^^^^^^^^^^
putdcc <idx> <text> [-raw]
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends text to the idx specified. If -raw is specified, the text will be sent as is, without forced new lines or limits to line length.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^
dccbroadcast <message>
^^^^^^^^^^^^^^^^^^^^^^

  Description: sends a message to everyone on the party line across the botnet, in the form of "\*\*\* <message>" for local users, "\*\*\* (Bot) <message>" for users on other bots with version below 1.8.4, and "(Bot) <message>" for users on other bots with version 1.8.4+ and console log mode 'l' enabled

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dccputchan <channel> <message>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends your message to everyone on a certain channel on the botnet, in a form exactly like dccbroadcast does. Valid channels are 0 through 99999.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^
boot <user@bot> [reason]
^^^^^^^^^^^^^^^^^^^^^^^^
  Description: boots a user from the partyline

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^
dccsimul <idx> <text>
^^^^^^^^^^^^^^^^^^^^^

  Description: simulates text typed in by the dcc user specified. Note that in v0.9, this only simulated commands; now a command must be preceded by a '.' to be simulated.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^
hand2idx <handle>
^^^^^^^^^^^^^^^^^

  Returns: the idx (a number greater than or equal to zero) for the user given if the user is on the party line in chat mode (even if she is currently on a channel or in chat off), the file area, or in the control of a script. -1 is returned if no idx is found. If the user is on multiple times, the oldest idx is returned.

  Module: core

^^^^^^^^^^^^^^
idx2hand <idx>
^^^^^^^^^^^^^^

  Returns: handle of the user with the given idx

  Module: core

^^^^^^^^^^^^^^
valididx <idx>
^^^^^^^^^^^^^^

  Returns: 1 if the idx currently exists; 0 otherwise

  Module: core

^^^^^^^^^^^^^
getchan <idx>
^^^^^^^^^^^^^

  Returns: the current party line channel for a user on the party line; "0" indicates he's on the group party line, "-1" means he has chat off, and a value from 1 to 99999 is a private channel

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^
setchan <idx> <channel>
^^^^^^^^^^^^^^^^^^^^^^^

  Description: sets a party line user's channel. The party line user is not notified that she is now on a new channel. A channel name can be used (provided it exists).

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
console <idx> [channel] [console-modes]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: changes a dcc user's console mode, either to an absolute mode (like "mpj") or just adding/removing flags (like "+pj" or "-moc" or "+mp-c"). The user's console channel view can be changed also (as long as the new channel is a valid channel).

  Returns: a list containing the user's (new) channel view and (new) console modes, or nothing if that user isn't currently on the partyline

  Module: core

^^^^^^^^^^^^^^^^^^
resetconsole <idx>
^^^^^^^^^^^^^^^^^^

  Description: changes a dcc user's console mode to the default setting in the configfile.

  Returns: a list containing the user's channel view and (new) console modes, or nothing if that user isn't currently on the partyline

  Module: core

^^^^^^^^^^^^^^^^^^^
echo <idx> [status]
^^^^^^^^^^^^^^^^^^^

  Description: turns a user's echo on or off; the status has to be a 1 or 0

  Returns: new value of echo for that user (or the current value, if status was omitted)

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
strip <idx> [+/-strip-flags]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: modifies the strip-flags for a user. The supported strip-flags are:

  +------+-------------------------------------------------------------+
  | c    | remove all color codes                                      |
  +------+-------------------------------------------------------------+
  | b    | remove all boldface codes                                   |
  +------+-------------------------------------------------------------+
  | r    | remove all reverse video codes                              |
  +------+-------------------------------------------------------------+
  | u    | remove all underline codes                                  |
  +------+-------------------------------------------------------------+
  | a    | remove all ANSI codes                                       |
  +------+-------------------------------------------------------------+
  | g    | remove all ctrl-g (bell) codes                              |
  +------+-------------------------------------------------------------+
  | o    | remove all ordinary codes (ctrl+o, terminates bold/color/..)|
  +------+-------------------------------------------------------------+
  | i    | remove all italics codes                                    |
  +------+-------------------------------------------------------------+
  | \*   | remove all of the above                                     |
  +------+-------------------------------------------------------------+

  Returns: new strip-flags for the specified user (or the current flags, if strip-flags was omitted)

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^
putbot <bot-nick> <message>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sends a message across the botnet to another bot. If no script intercepts the message on the other end, the message is ignored.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^
putallbots <message>
^^^^^^^^^^^^^^^^^^^^

  Description: sends a message across the botnet to all bots. If no script intercepts the message on the other end, the message is ignored.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^
killdcc <idx>
^^^^^^^^^^^^^

  Description: kills a partyline or file area connection

  Returns: nothing

  Module: core

^^^^
bots
^^^^

  Returns: list of the bots currently connected to the botnet

  Module: core

^^^^^^^
botlist
^^^^^^^

  Returns: a list of bots currently on the botnet. Each item in the list is a sublist with four elements: bot, uplink, version, and sharing status:

  +----------+-----------------------------------------------+
  | bot      | the bot's botnetnick                          |
  +----------+-----------------------------------------------+
  | uplink   | the bot the bot is connected to               |
  +----------+-----------------------------------------------+
  | version  | it's current numeric version                  |
  +----------+-----------------------------------------------+
  | sharing  | a "+" if the bot is a sharebot; "-" otherwise |
  +----------+-----------------------------------------------+

  Module: core

^^^^^^^^^^^^^^
islinked <bot>
^^^^^^^^^^^^^^

  Returns: 1 if the bot is currently linked; 0 otherwise

  Module: core

^^^^^^^
dccused
^^^^^^^

  Returns: number of dcc connections currently in use

  Module: core

^^^^^^^^^^^^^^
dcclist [type]
^^^^^^^^^^^^^^

  Returns: a list of active connections, each item in the list is a sublist containing six elements:
  {<idx> <handle> <hostname> <type> {<other>} <timestamp>}.

  The types are: chat, bot, files, file_receiving, file_sending, file_send_pending, script, socket (these are connections that have not yet been put under 'control'), telnet, and server. The timestamp is in unixtime format.

  Module: core

^^^^^^^^^^^
whom <chan>
^^^^^^^^^^^

  Returns: list of people on the botnet who are on that channel. 0 is the default party line. Each item in the list is a sublist with six elements: nickname, bot, hostname, access flag ('-', '@', '+', or '*'), minutes idle, and away message (blank if the user is not away). If you specify * for channel, every user on the botnet is returned with an extra argument indicating the channel the user is on.

  Module: core

^^^^^^^^^^^^^^^^
getdccidle <idx>
^^^^^^^^^^^^^^^^

  Returns: number of seconds the dcc chat/file system/script user has been idle

  Module: core

^^^^^^^^^^^^^^^^
getdccaway <idx>
^^^^^^^^^^^^^^^^

  Returns: away message for a dcc chat user (or "" if the user is not set away)

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^
setdccaway <idx> <message>
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sets a party line user's away message and marks them away. If set to "", the user is marked as no longer away.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^
connect <host> <[+]port>
^^^^^^^^^^^^^^^^^^^^^^^^

  Description: makes an outgoing connection attempt and creates a dcc entry for it. A 'control' command should be used immediately after a successful 'connect' so no input is lost. If the port is prefixed with a plus sign, SSL encrypted connection will be attempted.

  Returns: idx of the new connection

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
listen <port> <type> [options] [flag]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: opens a listening port to accept incoming telnets; type must be one of "bots", "all", "users", "script", or "off". Prefixing the port with a plus sign will make eggdrop accept SSL connections on it.

    listen <port> bots [mask]

      Description: accepts connections from bots only; the optional mask is used to identify permitted bot names. If the mask begins with '@', it is interpreted to be a mask of permitted hosts to accept connections from.

      Returns: port number

    listen <port> users [mask]
    
      Description: accepts connections from users only (no bots); the optional mask is used to identify permitted nicknames. If the mask begins with '@', it is interpreted to be a mask of permitted hosts to accept connections from.

      Returns: port number

    listen <port> all [mask]

      Description: accepts connections from anyone; the optional mask is used to identify permitted nicknames/botnames. If the mask begins with '@', it is interpreted to be a mask of permitted hosts to accept connections from.

      Returns: port number

    listen <port> script <proc> [flag]

      Description: accepts connections which are immediately routed to a proc. The proc is called with one parameter: the idx of the new connection. Flag may currently only be 'pub', which makes the bot allow anyone to connect and not perform an ident lookup.

      Returns: port number

    listen <port> off

      Description: stop listening on a port

      Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dccdumpfile <idx> <filename>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: dumps out a file from the text directory to a dcc chat user. The flag matching that's used everywhere else works here, too.

  Returns: nothing

  Module: core

Notes Module
------------

^^^^^^^^^^^^^^^^^^^^^^^^^
notes <user> [numberlist]
^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: -1 if no such user, -2 if notefile failure. If a numberlist is not specified, the number of notes stored for the user is returned. Otherwise, a list of sublists containing information about notes stored for the user is returned. Each sublist is in the format of::

        {<from> <timestamp> <note text>}

  Module: notes

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
erasenotes <user> <numberlist>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: erases some or all stored notes for a user. Use '-' to erase all notes.

  Returns: -1 if no such user, -2 if notefile failure, 0 if no such note, or number of erased notes.

  Module: notes

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
listnotes <user> <numberlist>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: lists existing notes according to the numberlist (ex: "2-4;8;16-")

  Returns: -1 if no such user, -2 if notefile failure, 0 if no such note, list of existing notes.

  Module: notes

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
storenote <from> <to> <msg> <idx>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: stores a note for later reading, notifies idx of any results (use idx -1 for no notify).

  Returns: 0 on success; non-0 on failure

  Module: notes

Assoc Module
------------

^^^^^^^^^^^^^^^^^^^
assoc <chan> [name]
^^^^^^^^^^^^^^^^^^^

  Description: sets the name associated with a botnet channel, if you specify one

  Returns: current name for that channel, if any

  Module: assoc

^^^^^^^^^^^^^^^^
killassoc <chan>
^^^^^^^^^^^^^^^^

  Description: removes the name associated with a botnet channel, if any exists. Use 'killassoc &' to kill all assocs.

  Returns: nothing

  Module: assoc

Compress Module
---------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
compressfile [-level <level>] <src-file> [target-file]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
and
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
uncompressfile <src-file> [target-file]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: compresses or un-compresses files. The level option specifies the compression mode to use when compressing. Available modes are from 0 (minimum CPU usage, minimum compression) all the way up to 9 (maximum CPU usage, maximum compression). If you don't specify the target-file, the src-file will be overwritten.

  Returns: nothing

  Module: compress

^^^^^^^^^^^^^^^^^^^^^^^
iscompressed <filename>
^^^^^^^^^^^^^^^^^^^^^^^

  Description: determines whether <filename> is gzip compressed. 

  Returns: 1 if it is, 0 if it isn't, and 2 if some kind of error prevented the checks from succeeding.

  Module: compress

Filesys Module
--------------

^^^^^^^^^^^^^^^^^^
setpwd <idx> <dir>
^^^^^^^^^^^^^^^^^^

  Description: changes the directory of a file system user, in exactly the same way as a 'cd' command would. The directory can be specified relative or absolute.

  Returns: nothing

  Module: filesys

^^^^^^^^^^^^
getpwd <idx>
^^^^^^^^^^^^

  Returns: the current directory of a file system user

  Module: filesys

^^^^^^^^^^^^^^
getfiles <dir>
^^^^^^^^^^^^^^

  Returns: a list of files in the directory given; the directory is relative to dcc-path

  Module: filesys

^^^^^^^^^^^^^
getdirs <dir>
^^^^^^^^^^^^^

  Returns: a list of subdirectories in the directory given; the directory is relative to dcc-path

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dccsend <filename> <ircnick>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: attempts to start a dcc file transfer to the given nick; the filename must be specified either by full pathname or in relation to the bot's startup directory

  Returns:

  +-------+---------------------------------------------------------------------+
  | 0     | success                                                             |
  +-------+---------------------------------------------------------------------+
  | 1     | the dcc table is full (too many connections)                        |
  +-------+---------------------------------------------------------------------+
  | 2     | can't open a socket for the transfer                                |
  +-------+---------------------------------------------------------------------+
  | 3     | the file doesn't exist                                              |
  +-------+---------------------------------------------------------------------+
  | 4     | the file was queued for later transfer, which means that person has |
  |       | too many file transfers going right now                             |
  +-------+---------------------------------------------------------------------+
  | 5     | copy-to-tmp is enabled and the file already exists in the temp      |
  |       | directory                                                           |
  +-------+---------------------------------------------------------------------+

  Module: transfer

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
filesend <idx> <filename> [ircnick]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: like dccsend, except it operates for a current filesystem user, and the filename is assumed to be a relative path from that user's current directory

  Returns: 0 on failure; 1 on success (either an immediate send or a queued send)

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
fileresend <idx> <filename> [ircnick]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: functions like filesend, only that it sends a DCC RESEND instead of a DCC SEND, which allows people to resume aborted file transfers if their client supports that protocol. ircII/BitchX/etc. support it; mIRC does not.

  Returns: 0 on failure; 1 on success (either an immediate send or a queued send)

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^^^^^^^
setdesc <dir> <file> <desc>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sets the description for a file in a file system directory; the directory is relative to dcc-path

  Returns: nothing

  Module: filesys

^^^^^^^^^^^^^^^^^^^^
getdesc <dir> <file>
^^^^^^^^^^^^^^^^^^^^

  Returns: the description for a file in the file system, if one exists

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setowner <dir> <file> <handle>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: changes the owner for a file in the file system; the directory is relative to dcc-path

  Returns: nothing

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^
getowner <dir> <file>
^^^^^^^^^^^^^^^^^^^^^

  Returns: the owner of a file in the file system

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^^^^^^^
setlink <dir> <file> <link>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: creates or changes a linked file (a file that actually exists on another bot); the directory is relative to dcc-path

  Returns: nothing

  Module: filesys

^^^^^^^^^^^^^^^^^^^^
getlink <dir> <file>
^^^^^^^^^^^^^^^^^^^^

  Returns: the link for a linked file, if it exists

  Module: filesys

^^^^^^^^^^^^^^^^^
getfileq <handle>
^^^^^^^^^^^^^^^^^

  Returns: list of files queued by someone; each item in the list will be a sublist with two elements: nickname the file is being sent to and the filename

  Module: transfer

^^^^^^^^^^^^^^^^^^^^^
getfilesendtime <idx>
^^^^^^^^^^^^^^^^^^^^^

  Returns: the unixtime value from when a file transfer started, or a negative number:

  +-----+------------------------------------------------------+
  | -1  | no matching transfer with the specified idx was found|
  +-----+------------------------------------------------------+
  | -2  | the idx matches an entry which is not a file transfer|
  +-----+------------------------------------------------------+

  Module: transfer

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
mkdir <directory> [<required-flags> [channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: creates a directory in the file system. Only users with the required flags may access it.

  Returns:

  +-----+------------------------------------------------------+
  | 0   | success                                              |
  +-----+------------------------------------------------------+
  | 1   | can't create directory                               |
  +-----+------------------------------------------------------+
  | 2   | directory exists but is not a directory              |
  +-----+------------------------------------------------------+
  | -3  | could not open filedb                                |
  +-----+------------------------------------------------------+

  Module: filesys

^^^^^^^^^^^^^^^^^
rmdir <directory>
^^^^^^^^^^^^^^^^^

  Description: removes a directory from the file system.

  Returns: 0 on success; 1 on failure

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^^^
mv <file> <destination>
^^^^^^^^^^^^^^^^^^^^^^^

  Description: moves a file from its source to the given destination. The file can also be a mask, such as /incoming/\*, provided the destination is a directory.

  Returns: If the command was successful, the number of files moved will be returned. Otherwise, a negative number will be returned:

  +-----+------------------------------------------------------+
  | -1  | invalid source file                                  |
  +-----+------------------------------------------------------+
  | -2  | invalid destination                                  |
  +-----+------------------------------------------------------+
  | -3  | destination file exists                              |
  +-----+------------------------------------------------------+
  | -4  | no matches found                                     |
  +-----+------------------------------------------------------+

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^^^
cp <file> <destination>
^^^^^^^^^^^^^^^^^^^^^^^

  Description: copies a file from its source to the given destination. The file can also be a mask, such as /incoming/\*, provided the destination is a directory.

  Returns: If the command was successful, the number of files copied will be returned. Otherwise, a negative number will be returned:

  +-----+------------------------------------------------------+
  | -1  | invalid source file                                  |
  +-----+------------------------------------------------------+
  | -2  | invalid destination                                  |
  +-----+------------------------------------------------------+
  | -3  | destination file exists                              |
  +-----+------------------------------------------------------+
  | -4  | no matches found                                     |
  +-----+------------------------------------------------------+

  Module: filesys

^^^^^^^^^^^^^^
getflags <dir>
^^^^^^^^^^^^^^

  Returns: the flags required to access a directory

  Module: filesys

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setflags <dir> [<flags> [channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: sets the flags required to access a directory

  Returns: 0 on success; -1 or -3 on failure

  Module: filesys

Miscellaneous Commands
----------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
bind <type> <flags> <keyword/mask> [proc-name]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: You can use the 'bind' command to attach Tcl procedures to certain events. flags are the flags the user must have to trigger the event (if applicable). proc-name is the name of the Tcl procedure to call for this command (see below for the format of the procedure call). If the proc-name is omitted, no binding is added. Instead, the current binding is returned (if it's stackable, a list of the current bindings is returned).

  Returns: name of the command that was added, or (if proc-name was omitted), a list of the current bindings for this command

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
unbind <type> <flags> <keyword/mask> <proc-name>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: removes a previously created bind

  Returns: name of the command that was removed

  Module: core

^^^^^^^^^^^^^^^^^
binds [type/mask]
^^^^^^^^^^^^^^^^^

  Returns: a list of Tcl binds, each item in the list is a sublist of five elements:
        {<type> <flags> <name> <hits> <proc>}

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
logfile [<modes> <channel> <filename>]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: creates a new logfile, which will log the modes given for the channel listed. If no logfile is specified, a list of existing logfiles will be returned. "*" indicates all channels. You can also change the modes and channel of an existing logfile with this command. Entering a blank mode and channel ("") makes the bot stop logging there.

  Logfile flags:

  +-----+---------------------------------------------------------------------+
  | b   | information about bot linking and userfile sharing                  |
  +-----+---------------------------------------------------------------------+
  | c   | commands                                                            |
  +-----+---------------------------------------------------------------------+
  | d   | misc debug information                                              |
  +-----+---------------------------------------------------------------------+
  | g   | raw outgoing share traffic                                          |
  +-----+---------------------------------------------------------------------+
  | h   | raw incoming share traffic                                          |
  +-----+---------------------------------------------------------------------+
  | j   | joins, parts, quits, topic changes, and netsplits on the channel    |
  +-----+---------------------------------------------------------------------+
  | k   | kicks, bans, and mode changes on the channel                        |
  +-----+---------------------------------------------------------------------+
  | l   | linked bot messages                                                 |
  +-----+---------------------------------------------------------------------+
  | m   | private msgs, notices and ctcps to the bot                          |
  +-----+---------------------------------------------------------------------+
  | o   | misc info, errors, etc (IMPORTANT STUFF)                            |
  +-----+---------------------------------------------------------------------+
  | p   | public text on the channel                                          |
  +-----+---------------------------------------------------------------------+
  | r   | raw incoming server traffic                                         |
  +-----+---------------------------------------------------------------------+
  | s   | server connects, disconnects, and notices                           |
  +-----+---------------------------------------------------------------------+
  | t   | raw incoming botnet traffic                                         |
  +-----+---------------------------------------------------------------------+
  | u   | raw outgoing botnet traffic                                         |
  +-----+---------------------------------------------------------------------+
  | v   | raw outgoing server traffic                                         |
  +-----+---------------------------------------------------------------------+
  | w   | wallops (make sure the bot sets +w in init-server)                  |
  +-----+---------------------------------------------------------------------+
  | x   | file transfers and file-area commands                               |
  +-----+---------------------------------------------------------------------+

  Returns: filename of logfile created, or, if no logfile is specified, a list of logfiles such as: {mco * eggdrop.log} {jp #lame lame.log}

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
maskhost <nick!user@host> [masktype]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: masked hostmask for the string given according to the masktype (the default is 3).

  Available types are:

  +-----+------------------------------------------------------+
  | 0   | \*!user\@host                                        |
  +-----+------------------------------------------------------+
  | 1   | \*!*user\@host                                       |
  +-----+------------------------------------------------------+
  | 2   | \*!*\@host                                           |
  +-----+------------------------------------------------------+
  | 3   | \*!*user\@*.host                                     |
  +-----+------------------------------------------------------+
  | 4   | \*!*\@*.host                                         |
  +-----+------------------------------------------------------+
  | 5   | nick!user\@host                                      |
  +-----+------------------------------------------------------+
  | 6   | nick!*user\@host                                     |
  +-----+------------------------------------------------------+
  | 7   | nick!*\@host                                         |
  +-----+------------------------------------------------------+
  | 8   | nick!*user\@*.host                                   |
  +-----+------------------------------------------------------+
  | 9   | nick!*\@*.host                                       |
  +-----+------------------------------------------------------+

  You can also specify types from 10 to 19 which correspond to types
  0 to 9, but instead of using a * wildcard to replace portions of the
  host, only numbers in hostnames are replaced with the '?' wildcard.
  Same is valid for types 20-29, but instead of '?', the '*' wildcard
  will be used. Types 30-39 set the host to '*'.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
timer <minutes> <tcl-command> [count]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: executes the given Tcl command after a certain number of minutes have passed. If count is specified, the command will be executed count times with the given interval in between. If you specify a count of 0, the timer will repeat until it's removed with killtimer or until the bot is restarted.

  Returns: a timerID

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
utimer <seconds> <tcl-command> [count]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: executes the given Tcl command after a certain number of seconds have passed. If count is specified, the command will be executed count times with the given interval in between. If you specify a count of 0, the utimer will repeat until it's removed with killutimer or until the bot is restarted.

  Returns: a timerID

  Module: core

^^^^^^
timers
^^^^^^

  Returns: a list of active minutely timers. Each entry in the list contains the number of minutes left till activation, the command that will be executed, the timerID, and the remaining number of repeats.

  Module: core

^^^^^^^
utimers
^^^^^^^

  Returns: a list of active secondly timers. Each entry in the list contains the number of minutes left till activation, the command that will be executed, the timerID, and the remaining number of repeats.

  Module: core

^^^^^^^^^^^^^^^^^^^
killtimer <timerID>
^^^^^^^^^^^^^^^^^^^

  Description: removes a minutely timer from the list

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^
killutimer <timerID>
^^^^^^^^^^^^^^^^^^^^

  Description: removes a secondly timer from the list

  Returns: nothing

  Module: core

^^^^^^^^
unixtime
^^^^^^^^

  Returns: a long integer which represents the number of seconds that have passed since 00:00 Jan 1, 1970 (GMT).

  Module: core

^^^^^^^^^^^^^^^^^^
duration <seconds>
^^^^^^^^^^^^^^^^^^

  Returns: the number of seconds converted into years, weeks, days, hours, minutes, and seconds. 804600 seconds is turned into 1 week 2 days 7 hours 30 minutes.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
strftime <formatstring> [time]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: a formatted string of time using standard strftime format. If time is specified, the value of the specified time is used. Otherwise, the current time is used.

  Module: core

^^^^^^^^^^^^^^^^
ctime <unixtime>
^^^^^^^^^^^^^^^^

  Returns: a formatted date/time string based on the current locale settings from the unixtime string given; for example "Fri Aug 3 11:34:55 1973"

  Module: core

^^^^
myip
^^^^

  Returns: a long number representing the bot's IP address, as it might appear in (for example) a DCC request

  Module: core

^^^^^^^^^^^^
rand <limit>
^^^^^^^^^^^^

  Returns: a random integer between 0 and limit-1. Limit must be greater than 0 and equal to or less than RAND_MAX, which is generally 2147483647. The underlying pseudo-random number generator is not cryptographically secure.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^
control <idx> <command>
^^^^^^^^^^^^^^^^^^^^^^^

  Description: removes an idx from the party line and sends all future input to the Tcl command given. The command will be called with two parameters: the idx and the input text. The command should return 0 to indicate success and 1 to indicate that it relinquishes control of the user back to the bot. If the input text is blank (""), it indicates that the connection has been dropped. Also, if the input text is blank, never call killdcc on it, as it will fail with "invalid idx".

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
sendnote <from> <to[@bot]> <message>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: simulates what happens when one user sends a note to another

  Returns:

  +-----+----------------------------------------------------------+
  | 0   | the send failed                                          |
  +-----+----------------------------------------------------------+
  | 1   | the note was delivered locally or sent to another bot    |
  +-----+----------------------------------------------------------+
  | 2   | the note was stored locally                              |
  +-----+----------------------------------------------------------+
  | 3   | the user's notebox is too full to store a note           |
  +-----+----------------------------------------------------------+
  | 4   | a Tcl binding caught the note                            |
  +-----+----------------------------------------------------------+
  | 5   | the note was stored because the user is away             |
  +-----+----------------------------------------------------------+

  Module: core

^^^^^^^^^^^^^^^^^^^^
link [via-bot] <bot>
^^^^^^^^^^^^^^^^^^^^

  Description: attempts to link to another bot directly. If you specify a via-bot, it tells the via-bot to attempt the link.

  Returns: 1 if the link will be attempted; 0 otherwise

  Module: core

^^^^^^^^^^^^
unlink <bot>
^^^^^^^^^^^^

  Description: attempts to unlink a bot from the botnet

  Returns: 1 on success; 0 otherwise

  Module: core

^^^^^^^^^^^^^^^^^^^^^^
encrypt <key> <string>
^^^^^^^^^^^^^^^^^^^^^^

  Returns: encrypted string (using the currently loaded encryption module), encoded into ASCII using base-64. As of v1.8.4, the default blowfish encryption module can use either the older ECB mode (currently used by default for compatibility reasons), or the more recent and more-secure CBC mode. You can explicitly request which encryption mode to use by prefixing the encryption key with either "ecb:" or "cbc:", or by using the blowfish-use-mode setting in the config file. Note: the default encryption mode for this function is planned to transition from ECB to CBC in v1.9.0.

  Module: encryption

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
decrypt <key> <encrypted-base64-string>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Returns: decrypted string (using the currently loaded encryption module). If the default blowfish encryption module is used, this automatically picks the right decryption mode. You may still prefix the key with "ecb:" or "cbc:" or use the blowfish-use-mode setting in the config file (see the encrypt command for more detailed information).

  Module: encryption

^^^^^^^^^^^^^^^^^^
encpass <password>
^^^^^^^^^^^^^^^^^^

  Returns: encrypted string (using the currently loaded encryption module)

  Module: encryption

^^^^^^^^^^^^
die [reason]
^^^^^^^^^^^^

  Description: causes the bot to log a fatal error and exit completely. If no reason is given, "EXIT" is used.

  Returns: none

  Module: core

^^^^^^
unames
^^^^^^

  Returns: the current operating system the bot is using

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dnslookup <ip-address/hostname> <proc> [[arg1] [arg2] ... [argN]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: This issues an asynchronous dns lookup request. The command will block if dns module is not loaded; otherwise it will either return immediately or immediately call the specified proc (e.g. if the lookup is already cached).

  As soon as the request completes, the specified proc will be called as follows:

    <proc> <ipaddress> <hostname> <status> [[arg1] [arg2] ... [argN]]

  status is 1 if the lookup was successful and 0 if it wasn't. All additional parameters (called arg1, arg2 and argN above) get appended to the proc's other parameters.

  Returns: nothing

  Module: core

^^^^^^^^^^^^
md5 <string>
^^^^^^^^^^^^

  Returns: the 128 bit MD5 message-digest of the specified string

  Module: core

^^^^^^^^^^^^^^^^^
callevent <event>
^^^^^^^^^^^^^^^^^

  Description: triggers the evnt bind manually for a certain event. You can call arbitrary events here, even ones that are not pre-defined by Eggdrop. For example: callevent rehash, or callevent myownevent123.

  Returns: nothing

  Module: core

^^^^^^^
traffic
^^^^^^^

  Returns: a list of sublists containing information about the bot's traffic usage in bytes. Each sublist contains five elements: type, in-traffic today, in-traffic total, out-traffic today, out-traffic total (in that order).

  Module: core

^^^^^^^
modules
^^^^^^^
  Returns: a list of sublists containing information about the bot's currently loaded modules. Each sublist contains three elements: module, version, and dependencies. Each dependency is also a sublist containing the module name and version.

  Module: core

^^^^^^^^^^^^^^^^^^^
loadmodule <module>
^^^^^^^^^^^^^^^^^^^

  Description: attempts to load the specified module.

  Returns: "Already loaded." if the module is already loaded, "" if successful, or the reason the module couldn't be loaded.

  Module: core

^^^^^^^^^^^^^^^^^^^^^
unloadmodule <module>
^^^^^^^^^^^^^^^^^^^^^

  Description: attempts to unload the specified module.

  Returns: "No such module" if the module is not loaded, "" otherwise.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^
loadhelp <helpfile-name>
^^^^^^^^^^^^^^^^^^^^^^^^

  Description: attempts to load the specified help file from the help/ directory.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^
unloadhelp <helpfile-name>
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: attempts to unload the specified help file.

  Returns: nothing

  Module: core

^^^^^^^^^^
reloadhelp
^^^^^^^^^^

  Description: reloads the bot's help files.

  Returns: nothing

  Module: core

^^^^^^^
restart
^^^^^^^

  Description: rehashes the bot, kills all timers, reloads all modules, and reconnects the bot to the next server in its list.

  Returns: nothing

  Module: core

^^^^^^
rehash
^^^^^^

  Description: rehashes the bot

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
stripcodes <strip-flags> <string>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: strips specified control characters from the string given. strip-flags can be any combination of the following:

  +-----+-------------------------------------------------------------+
  | c   | remove all color codes                                      |
  +-----+-------------------------------------------------------------+
  | b   | remove all boldface codes                                   |
  +-----+-------------------------------------------------------------+
  | r   | remove all reverse video codes                              |
  +-----+-------------------------------------------------------------+
  | u   | remove all underline codes                                  |
  +-----+-------------------------------------------------------------+
  | a   | remove all ANSI codes                                       |
  +-----+-------------------------------------------------------------+
  | g   | remove all ctrl-g (bell) codes                              |
  +-----+-------------------------------------------------------------+
  | o   | remove all ordinary codes (ctrl+o, terminates bold/color/..)|
  +-----+-------------------------------------------------------------+
  | i   | remove all italics codes                                    |
  +-----+-------------------------------------------------------------+
  | \*  | remove all of the above                                     |
  +-----+-------------------------------------------------------------+

  Returns: the stripped string.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchaddr <hostmask> <address>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: checks if the address matches the hostmask given. The address should be in the form nick!user\@host.

  Returns: 1 if the address matches the hostmask, 0 otherwise.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchcidr <block> <address> <prefix>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: performs a cidr match on the specified ip addresses. IPv6 is supported, if enabled at compile time.

  Example: matchcidr 192.168.0.0 192.168.1.17 16

  Returns: 1 if the address matches the block prefix, 0 otherwise.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchstr <pattern> <string>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: checks if pattern matches string. Only two wildcards are supported: '*' and '?'. Matching is case-insensitive. This command is intended as a simplified alternative to Tcl's string match.  

  Returns: 1 if the pattern matches the string, 0 if it doesn't.

  Module: core

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
rfcequal <string1> <string2>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description: Checks if two strings are equal. Case is ignored, and this uses RFC1459 matching {}|~ == []\^, depending on the rfc_compliant setting.

  Returns: 1 if equal, 0 if not.

  Module: core

^^^^^^^^^^^^^
status [type]
^^^^^^^^^^^^^

  Description: provides eggdrop status information similar to the .status command in partyline. The available types of information are:

  +------+---------------------------------------------------------------------+
  | cpu  | total cpu time spent by eggdrop                                     |
  +------+---------------------------------------------------------------------+
  | mem  | dynamically allocated memory excluding the Tcl interpreter          |
  +------+---------------------------------------------------------------------+
  | cache| user entries cache hits (in %)                                      |
  +------+---------------------------------------------------------------------+
  | ipv6 | shows whether IPv6 support was compiled in                          |
  +------+---------------------------------------------------------------------+

  Returns: the requested information type or all information if type isn't specified. The format is a flat list of name-value pairs.

  Module: core

^^^^^^^^^^^
istls <idx>
^^^^^^^^^^^

  Description: checks if a connection is encrypted or cleartext. This command is available on TLS-enabled bots only.

  Returns: 1 if the idx is a TLS connection, 0 if it's plaintext.

  Module: core

^^^^^^^^^^^^^^
starttls <idx>
^^^^^^^^^^^^^^

  Description: establishes a secure (using TLS) connection over idx. The TLS connection should be first negotiated over the plaintext link, or using other means. Both parties must switch to TLS simultaneously. This command is available on TLS-enabled bots only.

  Returns: nothing

  Module: core

^^^^^^^^^^^^^^^
tlsstatus <idx>
^^^^^^^^^^^^^^^

  Description: provides information about an established TLS connection This includes certificate and cipher information as well as protocol version. This command is available on TLS-enabled bots only.

  Returns: a flat list of name-value pairs

  Module: core

Global Variables
----------------

NOTE: All config file variables are also global.

^^^^^^^
botnick
^^^^^^^

  Value: the current nickname the bot is using (for example: "Valis", "Valis0", etc.)

  Module: server

^^^^^^^
botname
^^^^^^^

  Value: the current nick!user\@host that the server sees (for example: "Valis!valis\@crappy.com")

  Module: server

^^^^^^
server
^^^^^^

  Value: the current server's real name (what server calls itself) and port bot is connected to (for example: "irc.math.ufl.edu:6667") Note that this does not necessarily match the servers internet address.

  Module: server

^^^^^^^^^^^^^
serveraddress
^^^^^^^^^^^^^
  Value: the current server's internet address (hostname or IP) and port bot is connected to. This will correspond to the entry in server list (for example: "eu.undernet.org:6667"). Note that this does not necessarily match the name server calls itself.

  Module: server

^^^^^^^
version
^^^^^^^
  Value: current bot version "1.1.2+pl1 1010201"; first item is the text version, to include a patch string if present, and second item is a numerical version

  Module: core

^^^^^^^^^^^
numversion*
^^^^^^^^^^^
  Value: the current numeric bot version (for example: "1010201"). Numerical version is in the format of "MNNRRPP", where:

  +------+---------------------------------------+
  | M    | major release number                  |
  +------+---------------------------------------+
  | NN   | minor release number                  |
  +------+---------------------------------------+
  | RR   | sub-release number                    |
  +------+---------------------------------------+
  | PP   | patch level for that sub-release      |
  +------+---------------------------------------+

  Module: core

^^^^^^
uptime
^^^^^^
  Value: the unixtime value for when the bot was started

  Module: core

^^^^^^^^^^^^^
server-online
^^^^^^^^^^^^^
  Value: the unixtime value for when the bot connected to its current server

  Module: server

^^^^^^^^
lastbind
^^^^^^^^
  Value: the last command binding which was triggered. This allows you to identify which command triggered a Tcl proc.

  Module: core

^^^^^^^
isjuped
^^^^^^^
  Value: 1 if bot's nick is juped(437); 0 otherwise

  Module: server

^^^^^^^
handlen
^^^^^^^
  Value: the value of the HANDLEN define in src/eggdrop.h

  Module: core

^^^^^^
config
^^^^^^
  Value: the filename of the config file Eggdrop is currently using

  Module: core

^^^^^^^^^^^^^
configureargs
^^^^^^^^^^^^^
  Value: a string (not list) of configure arguments in shell expansion (single quotes)

  Module: core

Binds
-----

You can use the 'bind' command to attach Tcl procedures to certain events.
For example, you can write a Tcl procedure that gets called every time a
user says "danger" on the channel.

Some bind types are marked as "stackable". That means that you can bind
multiple commands to the same trigger. Normally, for example, a bind such
as 'bind msg - stop msg:stop' (which makes a msg-command "stop" call the
Tcl proc "msg:stop") will overwrite any previous binding you had for the
msg command "stop". With stackable bindings, like 'msgm' for example,
you can bind the same command to multiple procs. When the bind is triggered,
ALL of the Tcl procs that are bound to it will be called. Raw binds are
triggered before builtin binds, as a builtin bind has the potential to
modify args.

To remove a bind, use the 'unbind' command. For example, to remove the
bind for the "stop" msg command, use 'unbind msg - stop msg:stop'.

^^^^^^^^^^
Bind Types
^^^^^^^^^^

The following is a list of bind types and how they work. Below each bind type is the format of the bind command, the list of arguments sent to the Tcl proc, and an explanation.

(1)  MSG

  bind msg <flags> <command> <proc>

  procname <nick> <user\@host> <handle> <text>

  Description: used for /msg commands. The first word of the user's msg is the command, and everything else becomes the text argument.

  Module: server

(2)  DCC

  bind dcc <flags> <command> <proc>

  procname <handle> <idx> <text>

  Description: used for partyline commands; the command is the first word and everything else becomes the text argument. The idx is valid until the user disconnects. After that, it may be reused, so be careful about storing an idx for long periods of time.

  Module: core

(3)  FIL

  bind fil <flags> <command> <proc>

  procname <handle> <idx> <text>

  Description: the same as DCC, except this is triggered if the user is in the file area instead of the party line

  Module: filesys

(4)  PUB

  bind pub <flags> <command> <proc>

  procname <nick> <user\@host> <handle> <channel> <text>

  Description: used for commands given on a channel. The first word becomes the command and everything else is the text argument.

  Module: irc

(5)  MSGM (stackable)

  bind msgm <flags> <mask> <proc>

  procname <nick> <user\@host> <handle> <text>

  Description: matches the entire line of text from a /msg with the mask. This is useful for binding Tcl procs to words or phrases spoken anywhere within a line of text. If the proc returns 1, Eggdrop will not log the message that triggered this bind. MSGM binds are processed before MSG binds. If the exclusive-binds setting is enabled, MSG binds will not be triggered by text that a MSGM bind has already handled.

  Module: server

(6)  PUBM (stackable)

  bind pubm <flags> <mask> <proc>

  procname <nick> <user\@host> <handle> <channel> <text>

  Description: just like MSGM, except it's triggered by things said on a channel instead of things /msg'd to the bot. The mask is matched against the channel name followed by the text and can contain wildcards. If the proc returns 1, Eggdrop will not log the message that triggered this bind. PUBM binds are processed before PUB binds. If the exclusive-binds setting is enabled, PUB binds will not be triggered by text that a PUBM bind has already handled.

  Examples:
    bind pubm * "#eggdrop Hello*" myProc
      Listens on #eggdrop for any line that begins with "Hello"
    bind pubm * "% Hello*" myProc
      Listens on any channel for any line that begins with "Hello"
    bind pubm * "% !command" myProc
      Listens on any channel for a line that ONLY contains "!command"
             
  Module: irc

(7)  NOTC (stackable)

  bind notc <flags> <mask> <proc>

  procname <nick> <user\@host> <handle> <text> <dest>

  Description: dest will be a nickname (the bot's nickname, obviously) or a channel name. mask is matched against the entire text of the notice and can contain wildcards. It is considered a breach of protocol to respond to a /notice on IRC, so this is intended for internal use (logging, etc.) only. Note that server notices do not trigger the NOTC bind. If the proc returns 1, Eggdrop will not log the message that triggered this bind.

  New Tcl procs should be declared as::

   proc notcproc {nick uhost hand text {dest ""}} {
     global botnick; if {$dest == ""} {set dest $botnick}
     ...
   }

  for compatibility.

  Module: server

(8)  JOIN (stackable)

  bind join <flags> <mask> <proc>

  procname <nick> <user\@host> <handle> <channel>

  Description: triggered by someone joining the channel. The mask in the bind is matched against "#channel nick!user\@host" and can contain wildcards.

  Module: irc

(9)  PART (stackable)

  bind part <flags> <mask> <proc>

  procname <nick> <user\@host> <handle> <channel> <msg>

  Description: triggered by someone leaving the channel. The mask is matched against "#channel nick!user\@host" and can contain wildcards. If no part message is specified, msg will be set to "".

  New Tcl procs should be declared as::

    proc partproc {nick uhost hand chan {msg ""}} { ... }

  for compatibility.

  Module: irc

(10) SIGN (stackable)

  bind sign <flags> <mask> <proc>
  
  procname <nick> <user\@host> <handle> <channel> <reason>

  Description: triggered by a signoff, or possibly by someone who got netsplit and never returned. The signoff message is the last argument to the proc. Wildcards can be used in the mask, which is matched against '#channel nick!user\@host'.

  Module: irc

(11) TOPC (stackable)

  bind topc <flags> <mask> <proc>
  
  procname <nick> <user\@host> <handle> <channel> <topic>

  Description: triggered by a topic change. mask can contain wildcards and is matched against '#channel <new topic>'.

  Module: irc

(12) KICK (stackable)

  bind kick <flags> <mask> <proc>
  
  procname <nick> <user\@host> <handle> <channel> <target> <reason>

  Description: triggered when someone is kicked off the channel. The mask is matched against '#channel target reason' where the target is the nickname of the person who got kicked (can contain wildcards). The proc is called with the nick, user\@host, and handle of the kicker, plus the channel, the nickname of the person who was kicked, and the reason.


  Module: irc

(13) NICK (stackable)

  bind nick <flags> <mask> <proc>
  
  procname <nick> <user\@host> <handle> <channel> <newnick>

  Description: triggered when someone changes nicknames. The mask is matched against '#channel newnick' and can contain wildcards. Channel is "*" if the user isn't on a channel (usually the bot not yet in a channel).

  Module: irc

(14) MODE (stackable)

  bind mode <flags> <mask> <proc>

  procname <nick> <user\@host> <handle> <channel> <mode-change> <target>

  Description: mode changes are broken down into their component parts before being sent here, so the <mode-change> will always be a single mode, such as "+m" or "-o". target will show the argument of the mode change (for o/v/b/e/I) or "" if the set mode does not take an argument. The bot's automatic response to a mode change will happen AFTER all matching Tcl procs are called. The mask will be matched against '#channel +/-modes' and can contain wildcards.

  If it is a server mode, nick will be "", user\@host is the server name, and handle is \*.
 
  Note that "target" was added in 1.3.17 and that this will break Tcl scripts that were written for pre-1.3.17 Eggdrop that use the mode binding. Also, due to a typo, mode binds were broken completely in 1.3.17 but were fixed in 1.3.18. Mode bindings are not triggered at all in 1.3.17.

  One easy example (from guppy) of how to support the "target" parameter in 1.3.18 and later and still remain compatible with older Eggdrop versions is:

  Old script looks as follows::

             bind mode - * mode_proc
             proc mode_proc {nick uhost hand chan mode} { ... }

  To make it work with 1.3.18+ and stay compatible with older bots, do::

             bind mode - * mode_proc_fix
             proc mode_proc_fix {nick uhost hand chan mode {target ""}} {
               if {$target != ""} {append mode " $target"}
               mode_proc $nick $uhost $hand $chan $mode
             }
             proc mode_proc {nick uhost hand chan mode} { ... }

  Module: irc

(15) CTCP (stackable)

  bind ctcp <flags> <keyword> <proc>

  procname <nick> <user\@host> <handle> <dest> <keyword> <text>

  Description: dest will be a nickname (the bot's nickname, obviously) or channel name. keyword is the ctcp command (which can contain wildcards), and text may be empty. If the proc returns 0, the bot will attempt its own processing of the ctcp command.

  Module: server

(16) CTCR (stackable)

  bind ctcr <flags> <keyword> <proc>

  procname <nick> <user\@host> <handle> <dest> <keyword> <text>

  Description: just like ctcp, but this is triggered for a ctcp-reply (ctcp embedded in a notice instead of a privmsg)

  Module: server

(17) RAW (stackable)

  bind raw <flags> <keyword> <proc>

  procname <from> <keyword> <text>

  Description: previous versions of Eggdrop required a special compile option to enable this binding, but it's now standard. The keyword is either a numeric, like "368", or a keyword, such as "PRIVMSG". "from" will be the server name or the source user (depending on the keyword); flags are ignored. The order of the arguments is identical to the order that the IRC server sends to the bot. The pre-processing  only splits it apart enough to determine the keyword. If the proc returns 1, Eggdrop will not process the line any further (this could cause unexpected behavior in some cases).

  Module: server

(18) BOT

  bind bot <flags> <command> <proc>

  procname <from-bot> <command> <text>

  Description: triggered by a message coming from another bot in the botnet. The first word is the command and the rest becomes the text argument; flags are ignored.

  Module: core

(19) CHON (stackable)

  bind chon <flags> <mask> <proc>

  procname <handle> <idx>

  Description: when someone first enters the party-line area of the bot via dcc chat or telnet, this is triggered before they are connected to a chat channel (so, yes, you can change the channel in a 'chon' proc). mask is matched against the handle and supports wildcards. This is NOT triggered when someone returns from the file area, etc.

  Module: core

(20) CHOF (stackable)

  bind chof <flags> <mask> <proc>

  procname <handle> <idx>

  Description: triggered when someone leaves the party line to disconnect from the bot. mask is matched against the handle and can contain wildcards. Note that the connection may have already been dropped by the user, so don't send output to the idx.

  Module: core

(21) SENT (stackable)

  bind sent <flags> <mask> <proc>

  procname <handle> <nick> <path/to/file>

  Description: after a user has successfully downloaded a file from the bot, this binding is triggered. mask is matched against the handle of the user that initiated the transfer and supports wildcards. nick is the actual recipient (on IRC) of the file. The path is relative to the dcc directory (unless the file transfer was started by a script call to 'dccsend', in which case the path is the exact path given in the call to 'dccsend').

  Module: transfer

(22) RCVD (stackable)

  bind rcvd <flags> <mask> <proc>

  procname <handle> <nick> <path/to/file>

  Description: triggered after a user uploads a file successfully. mask is matched against the user's handle. nick is the IRC nickname that the file transfer originated from. The path is where the file ended up, relative to the dcc directory (usually this is your incoming dir).

  Module: transfer

(23) CHAT (stackable)

  bind chat <flags> <mask> <proc>

  procname <handle> <channel#> <text>

  Description: when a user says something on the botnet, it invokes this binding. Flags are ignored; handle could be a user on this bot ("DronePup") or on another bot ("Eden\@Wilde") and therefore you can't rely on a local user record. The mask is checked against the entire line of text and supports wildcards.

  NOTE: If a BOT says something on the botnet, the BCST bind is invoked instead.

  Module: core

(24) LINK (stackable)

  bind link <flags> <mask> <proc>

  procname <botname> <via>

  Description: triggered when a bot links into the botnet. botname is the botnetnick of the bot that just linked in; via is the bot it linked through. The mask is checked against the botnetnick of the bot that linked and supports wildcards. flags are ignored.

  Module: core

(25) DISC (stackable)

  bind disc <flags> <mask> <proc>

  procname <botname>

  Description: triggered when a bot disconnects from the botnet for whatever reason. Just like the link bind, flags are ignored; mask is matched against the botnetnick of the bot that unlinked. Wildcards are supported in mask.

  Module: core

(26) SPLT (stackable)

  bind splt <flags> <mask> <proc>

  procname <nick> <user\@host> <handle> <channel>

  Description: triggered when someone gets netsplit on the channel. Be aware that this may be a false alarm (it's easy to fake a netsplit signoff message on some networks); mask may contain wildcards and is matched against '#channel nick!user\@host'. Anyone who is SPLT will trigger a REJN or SIGN within the next wait-split (defined in the config file) seconds.

  Module: irc

(27) REJN (stackable)

  bind rejn <flags> <mask> <proc>

  procname <nick> <user\@host> <handle> <channel>

  Description: someone who was split has rejoined. mask can contain wildcards, and is matched against '#channel nick!user\@host'.

  Module: irc

(28) FILT (stackable)

  bind filt <flags> <mask> <proc>

  procname <idx> <text>

  Description: party line and file system users have their text sent through filt before being processed. If the proc returns a blank string, the text is considered parsed. Otherwise, the bot will use the text returned from the proc and continue parsing that

  Module: core

(29) NEED (stackable)

  bind need <flags> <mask> <proc>

  procname <channel> <type>

  Description: this bind is triggered on certain events, like when the bot needs operator status or the key for a channel. The types are: op, unban, invite, limit, and key; the mask is matched against '#channel type' and can contain wildcards. flags are ignored.

  Example::

    bind need - "% op" needop < handles only need op
    bind need - "*" needall   < handles all needs

  Module: irc

(30) FLUD (stackable)

  bind flud <flags> <type> <proc>

  procname <nick> <user\@host> <handle> <type> <channel>

  Description: any floods detected through the flood control settings (like 'flood-ctcp') are sent here before processing. If the proc returns 1, no further action is taken on the flood; if the proc returns 0, the bot will do its normal "punishment" for the flood. The flood types are: pub, msg, join, or ctcp (and can be masked to "*" for the bind); flags are ignored.

  Module: server

(31) NOTE (stackable)

  bind note <flags> <mask> <proc>

  procname <from> <to> <text>

  Description: incoming notes (either from the party line, someone on IRC, or someone on another bot on the botnet) are checked against these binds before being processed. The mask is matched against the receiving handle and supports wildcards. If the proc returns 1, Eggdrop will not process the note any further. Flags are ignored.

  Module: core

(32) ACT (stackable)

  bind act <flags> <mask> <proc>

  procname <handle> <channel#> <action>

  Description: when someone does an action on the botnet, it invokes this binding. flags are ignored; the mask is matched against the text of the action and can support wildcards.

  Module: core

(33) WALL (stackable)

  bind wall <flags> <mask> <proc>

  procname <from> <msg>

  Description: when the bot receives a wallops, it invokes this binding. flags are ignored; the mask is matched against the text of the wallops msg. Note that RFC shows the server name as a source of the message, whereas many IRCds send the nick!user\@host of the actual sender, thus, Eggdrop will not parse it at all, but simply pass it to bind in its original form. If the proc returns 1,           Eggdrop will not log the message that triggered this bind.

  Module: server

(34) BCST (stackable)

  bind bcst <flags> <mask> <proc>

  procname <botname> <channel#> <text>
 
  Description: when a bot broadcasts something on the botnet (see 'dccbroadcast' above), it invokes this binding. flags are ignored; the mask is matched against the message text and can contain wildcards. 'channel' argument will always be '-1' since broadcasts are not directed to any partyline channel.
 
  It is also invoked when a BOT (not a person, as with the CHAT bind) 'says' something on a channel. In this case, the 'channel' argument will be a valid channel, and not '-1'.

  Module: core

(35) CHJN (stackable)

  bind chjn <flags> <mask> <proc>

  procname <botname> <handle> <channel#> <flag> <idx> <user\@host>

  Description: when someone joins a botnet channel, it invokes this binding. The mask is matched against the channel and can contain wildcards. flag is one of: * (owner), + (master), @ (op), or % (botnet master). Flags are ignored.

  Module: core

(36) CHPT (stackable)

  bind chpt <flags> <mask> <proc>

  procname <botname> <handle> <idx> <channel#>

  Description: when someone parts a botnet channel, it invokes this binding. The mask is matched against the channel and can contain wildcards. Flags are ignored.

  Module: core

(37) TIME (stackable)

  bind time <flags> <mask> <proc>

  procname <minute 00-59> <hour 00-23> <day 01-31> <month 00-11> <year 0000-9999>

  Description: allows you to schedule procedure calls at certain times. mask matches 5 space separated integers of the form: "minute hour day month year". The month var starts at 00 (Jan) and ends at 11 (Dec). Minute, hour, day, month have a zero padding so they are exactly two characters long; year is four characters. Flags are ignored.

  Module: core

(38) AWAY (stackable)

  bind away <flags> <mask> <proc>

  procname <botname> <idx> <text>

  Description: triggers when a user goes away or comes back on the botnet. text is the reason than has been specified (text is "" when returning). mask is matched against the botnet-nick of the bot the user is connected to and supports wildcards. flags are ignored.

  Module: core

(39) LOAD (stackable)

  bind load <flags> <mask> <proc>

  procname <module>

  Description: triggers when a module is loaded. mask is matched against the name of the loaded module and supports wildcards; flags are ignored.

  Module: core

(40) UNLD (stackable)

  bind unld <flags> <mask> <proc>

  procname <module>

  Description: triggers when a module is unloaded. mask is matched against the name of the unloaded module and supports wildcards;
  flags are ignored.

  Module: core

(41) NKCH (stackable)

  bind nkch <flags> <mask> <proc>

  procname <oldhandle> <newhandle>

  Description: triggered whenever a local user's handle is changed (in the userfile). mask is matched against the user's old handle and can contain wildcards; flags are ignored.

  Module: core

(42) EVNT (stackable)

  bind evnt <flags> <type> <proc>

  procname <type>

  Description: triggered whenever one of these events happen. flags are ignored. Pre-defined events triggered by Eggdrop are::

          sighup            - called on a kill -HUP <pid>
          sigterm           - called on a kill -TERM <pid>
          sigill            - called on a kill -ILL <pid>
          sigquit           - called on a kill -QUIT <pid>
          save              - called when the userfile is saved
          rehash            - called just after a rehash
          prerehash         - called just before a rehash
          prerestart        - called just before a restart
          logfile           - called when the logs are switched daily
          loaded            - called when the bot is done loading
          userfile-loaded   - called after userfile has been loaded
          connect-server    - called just before we connect to an IRC server
          preinit-server    - called immediately when we connect to the server
          init-server       - called when we actually get on our IRC server
          disconnect-server - called when we disconnect from our IRC server
          fail-server       - called when an IRC server fails to respond 

  Note that Tcl scripts can trigger arbitrary events, including ones that are not pre-defined or used by Eggdrop.

  Module: core

(43) LOST (stackable)

  bind lost <flags> <mask> <proc>

  procname <handle> <nick> <path> <bytes-transferred> <length-of-file>

  Description: triggered when a DCC SEND transfer gets lost, such as when the connection is terminated before all data was successfully sent/received. This is typically caused by a user abort.

  Module: transfer

(44) TOUT (stackable)

  bind tout <flags> <mask> <proc>

  procname <handle> <nick> <path> <bytes-transferred> <length-of-file>

  Description: triggered when a DCC SEND transfer times out. This may either happen because the dcc connection was not accepted or because the data transfer stalled for some reason.

  Module: transfer

(45) OUT (stackable)

  bind out <flags> <mask> <proc>

  procname <queue> <message> <queued|sent>

  Description: triggered whenever output is sent to the server. Normally the event will occur twice for each line sent: once before entering a server queue and once after the message is actually sent. This allows for more flexible logging of server output and introduces the ability to cancel the message. Mask is matched against "queue status", where status is either 'queued' or 'sent'. Queues are: mode, server, help, noqueue. noqueue is only used by the putnow tcl command.

  Module: server

(46) CRON (stackable)

  bind cron <flags> <mask> <proc>

  procname <minute 0-59> <hour 0-23> <day 1-31> <month 1-12> <weekday 0-6>

  Description: similar to bind TIME, but the mask is evaluated as a cron expression, e.g. "16/2 \*/2 5-15 7,8,9 4". It can contain up to five fields: minute, hour, day, month, weekday; delimited by whitespace. Week days are represented as 0-6, where Sunday can be either 0 or 7. Symbolic names are not supported. The bind will be triggered if the mask matches all of the fields, except that if both day and weekday are not '\*', only one of them is required to match. If any number of fields are omitted at the end, the match will proceed as if they were '\*'. All cron operators are supported. Please refer to the crontab manual for their meanings. Flags are ignored.

  Module: core

(47) LOG (stackable)

  bind log <flags> <mask> <proc>

  procname <level> <channel> <message>

  Description: triggered whenever a message is sent to a log. The mask is matched against "channel text". The level argument to the proc will contain the level(s) the message is sent to, or '\*' if the message is sent to all log levels at once. If the message wasn't sent to a specific channel, channel will be set to '\*'.

  Module: core

(48) TLS (stackable)

  bind tls <flags> <mask> <proc>

  procname <idx>
 
  Description: triggered for tcp connections when a ssl handshake has completed and the connection is secured. The mask is matched against the idx of the connection.

  Module: core

(49) DIE (stackable)

  bind die <flags> <mask> <proc>

  procname <shutdownreason>
 
  Description: triggered when eggdrop is about to die. The mask is matched against the shutdown reason. The bind won't be triggered if the bot crashes or is being terminated by SIGKILL.

  Module: core

^^^^^^^^^^^^^
Return Values
^^^^^^^^^^^^^

Several bindings pay attention to the value you return from the proc(using 'return <value>'). Usually, they expect a 0 or 1, and returning an empty return is interpreted as a 0. Be aware if you omit the return statement, the result of the last Tcl command executed will be returned by the proc. This will not likely produce the results you intended (this is a "feature" of Tcl).

Here's a list of the bindings that use the return value from procs they trigger:
 
(1) MSG   Return 1 to make Eggdrop log the command as:: 

    (nick!user@host) !handle! command

(2) DCC   Return 1 to make Eggdrop log the command as::

    #handle# command

(3) FIL   Return 1 to make Eggdrop log the command as::

    #handle# files: command

(4) PUB   Return 1 to make Eggdrop log the command as::

    <<nick>> !handle! command

(5) CTCP  Return 1 to ask the bot not to process the CTCP command on its own. Otherwise, it would send its own response to the CTCP (possibly an error message if it doesn't know how to deal with it).
  
(6) FILT  Return "" to indicate the text has been processed, and the bot should just ignore it. Otherwise, it will treat the text like any other.

(7) FLUD  Return 1 to ask the bot not to take action on the flood. Otherwise it will do its normal punishment.

(8) RAW   Return 1 to ask the bot not to process the server text. This can affect the bot's performance by causing it to miss things that it would normally act on -- you have been warned.

(9) CHON  Return 1 to ask the bot not to process the partyline join event.

(10) CHOF  Return 1 to ask the bot not to process the partyline part event.

(11) WALL  Return 1 to make Eggdrop not log the message that triggered this bind.

(12) NOTE  Return 1 to make Eggdrop not process the note any further. This includes stacked note bindings that would be processed after this one, as well as the built-in eggdrop note handling routines.

(13) MSGM  Return 1 to make Eggdrop not log the message that triggered this bind.

(14) PUBM  Return 1 to make Eggdrop not log the message that triggered this bind.

(15) NOTC  Return 1 to make Eggdrop not log the message that triggered this bind.

(16) OUT   Return 1 to make Eggdrop drop the message instead of sending it. Only meaningful for messages with status "queued".

(17) EVNT  Return 1 to make Eggdrop not to take the default action for the event. Used for signal type events, ignored for others.

(18) TLS   Return 1 to disable verbose ssl information for the handshake.

Control Procedures
------------------

Using the 'control' command, you can put a DCC connection (or outgoing
TCP connection) in control of a script. All text received from the
connection is sent to the proc you specify. All outgoing text should
be sent with 'putdcc'.

The control procedure is called with these parameters::

  procname <idx> <input-text>

This allows you to use the same proc for several connections. The
idx will stay the same until the connection is dropped. After that,
it will probably get reused for a later connection.

To indicate that the connection has closed, your control procedure
will be called with blank text (the input-text will be ""). This
is the only time it will ever be called with "" as the text, and it
is the last time your proc will be called for that connection. Don't
call killdcc on the idx when text is blank, it will always fail with
"invalid idx".

If you want to hand control of your connection back to Eggdrop, your
proc should return 1. Otherwise, return 0 to retain control.

TCP Connections
---------------

Eggdrop allows you to make two types of TCP ("telnet") connections:
outgoing and incoming. For an outgoing connection, you specify the
remote host and port to connect to. For an incoming connection, you
specify a port to listen on.

All of the connections are *event driven*. This means that the bot will
trigger your procs when something happens on the connection, and your
proc is expected to return as soon as possible. Waiting in a proc for
more input is a no-no.

To initiate an outgoing connection, use::

  set idx [connect <hostname> <[+]port>]
  
For SSL connections, prefix the port with a plus sign.

$idx now contains a new DCC entry for the outgoing connection.

All connections use non-blocking (commonly called "asynchronous",
which is a misnomer) I/O. Without going into a big song and dance
about asynchronous I/O, what this means to you is:

  * assume the connection succeeded immediately
  * if the connection failed, an EOF will arrive for that idx

The only time a 'connect' will return an error is if you give it a
hostname that can't be resolved (this is considered a "DNS error").
Otherwise, it will appear to have succeeded. If the connection failed,
you will immediately get an EOF.

Right after doing a 'connect' call, you should set up a 'control' for
the new idx (see the section above). From then on, the connection will
act just like a normal DCC connection that has been put under the control
of a script. If you ever return "1" from the control proc (indicating
that you want control to return to Eggdrop), the bot will just close the
connection and dispose of it. Other commands that work on normal DCC
connections, like 'killdcc' and 'putdcc', will work on this idx, too.
The 'killdcc' command will fail with "invalid idx" if you attempt to use
it on a closed socket.

To create a listen port, use::

  listen <[+]port> script <proc>

By default, a listen port will allow both plaintext and SSL connections.
To restrict a port to allow only SSL connections, prefix the port with a 
plus sign.
   
Procs should be declared as::

  <procname> <newidx>

For example::

  listen 6687 script listen:grab

  proc listen:grab {newidx} {
    control $newidx listen:control
  }

When a new connection arrives in port 6687, Eggdrop will create a new idx for the connection. That idx is sent to 'listen:grab'. The proc immediately puts this idx under control. Once 'listen:grab' has been called, the idx behaves exactly like an outgoing connection would.

Secure connection can be also established after a connection is active. You can connect/listen normally and switch later using the 'starttls' command. Your script should first inform the other side of the connection that it wants to switch to SSL. How to do this is application specific.

The best way to learn how to use these commands is to find a script that uses them and follow it carefully. However, hopefully this has given you a good start.

Match Characters
----------------

Many of the bindings allow match characters in the arguments. Here
are the four special characters:

+-----+--------------------------------------------------------------------------+
| ?   | matches any single character                                             |
+-----+--------------------------------------------------------------------------+
| \*  | matches 0 or more characters of any type                                 |
+-----+--------------------------------------------------------------------------+
| %   | matches 0 or more non-space characters (can be used to match a single    |
|     | word) (This character only works in binds, not in regular matching)      |
+-----+--------------------------------------------------------------------------+
| ~   | matches 1 or more space characters (can be used for whitespace between   |
|     | words) (This char only works in binds, not in regular matching)          |
+-----+--------------------------------------------------------------------------+

  Copyright (C) 1999 - 2019 Eggheads Development Team
