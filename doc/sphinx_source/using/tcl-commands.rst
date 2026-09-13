.. highlight:: text

Eggdrop Tcl Commands
Last revised: September 13, 2026

====================
Eggdrop Tcl Commands
====================


This is an exhaustive list of all the Tcl commands added to Eggdrop. All
of the normal Tcl built-in commands are still there, of course, but you
can also use these to manipulate features of the bot. They are listed
according to category.

This list is accurate for Eggdrop v1.10.2. Most scripts written for the v1.3, v1.4,
1.6, 1.8, and 1.9 series of Eggdrop should probably work in their current form, with only a very few needing minor modifications.
Scripts which were written for v0.9, v1.0, v1.1 or v1.2 will probably not work without modification.

.. note::

   **Command notation:** Command headings show the complete Tcl signature and can be linked to directly.
   ``<argument>`` denotes a required argument, while ``[argument]`` denotes an optional argument.
   Literal values, flags, identifiers, module names, and exact return values are shown in monospace.
   Examples are collapsed by default.

Output Commands
---------------

.. _tcl-putserv:

^^^^^^^^^^^^^^^^^^^^^^^^
putserv <text> [options]
^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends text to the server, like ``.dump`` (intended for direct server commands); output is queued so that the bot won't flood itself off the server.

  Options
     ``-next``
        Push messages to the front of the queue
     ``-normal``
        No effect

  Returns
     nothing


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putserv "PRIVMSG #lamest :Hello from LamestBot!"

----

.. _tcl-puthelp:

^^^^^^^^^^^^^^^^^^^^^^^^
puthelp <text> [options]
^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends text to the server, like ``putserv``, but it uses a different queue intended for sending messages to channels or people.

  Options
     ``-next``
        Push messages to the front of the queue
     ``-normal``
        No effect

  Returns
     nothing


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        puthelp "PRIVMSG #lamest :Hello, Foobar!"

----

.. _tcl-putquick:

^^^^^^^^^^^^^^^^^^^^^^^^^
putquick <text> [options]
^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends text to the server, like ``putserv``, but it uses a different (and faster) queue.

  Options
     ``-next``
        Push messages to the front of the queue
     ``-normal``
        No effect

  Returns
     nothing


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putquick "NOTICE Foobar :This message uses the quick queue."

----

.. _tcl-putnow:

^^^^^^^^^^^^^^^^^^^^^^^^
putnow <text> [-oneline]
^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends text to the server immediately, bypassing all queues. Use with caution, as the bot may easily flood itself off the server.

  Options
     ``-oneline``
        Send text up to the first \r or \n, discarding the rest

  Returns
     nothing


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putnow "PING :irc.example.net"

----

.. _tcl-putkick:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
putkick <channel> <nick,nick,...> [reason]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends kicks to the server and tries to put as many nicks into one kick command as possible.

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putkick "#lamest" Foobar "Example kick reason"

----

.. _tcl-putlog:

^^^^^^^^^^^^^
putlog <text>
^^^^^^^^^^^^^

  Description
     Logs ``<text>`` to the logfile and partyline if the ``misc`` flag (o) is active via the ``logfile`` config file setting and the ``.console`` partyline setting, respectively.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putlog "LamestBot loaded the example script."

----

.. _tcl-putcmdlog:

^^^^^^^^^^^^^^^^
putcmdlog <text>
^^^^^^^^^^^^^^^^

  Description
     Logs ``<text>`` to the logfile and partyline if the ``cmds`` flag (c) is active via the ``logfile`` config file setting and the ``.console`` partyline setting, respectively.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putcmdlog "Foobar used an example command."

----

.. _tcl-putxferlog:

^^^^^^^^^^^^^^^^^
putxferlog <text>
^^^^^^^^^^^^^^^^^

  Description
     Logs ``<text>`` to the logfile and partyline if the ``files`` flag (x) is active via the ``logfile`` config file setting and the ``.console`` partyline setting, respectively.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putxferlog "Example file transfer completed."

----

.. _tcl-putloglev:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
putloglev <flag(s)> <channel> <text>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Logs ``<text>`` to the logfile and partyline at the log level of the specified flag. Use ``*`` in lieu of a flag to indicate all log levels.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putloglev o "#lamest" "Example log message."

----

.. _tcl-dumpfile:

^^^^^^^^^^^^^^^^^^^^^^^^^^
dumpfile <nick> <filename>
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Dumps file from the help/text directory to a user on IRC via msg (one line per msg). The user has no flags, so the flag bindings won't work within the file.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        dumpfile Foobar help.txt

----

.. _tcl-queuesize:

^^^^^^^^^^^^^^^^^
queuesize [queue]
^^^^^^^^^^^^^^^^^

  Description
     Returns the number of messages waiting in Eggdrop's output queues.


  Returns
     the number of messages in all queues. If a queue is specified, only the size of this queue is returned. Valid queues are: mode, server, help.


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set queued [queuesize server]

----

.. _tcl-clearqueue:

^^^^^^^^^^^^^^^^^^
clearqueue <queue>
^^^^^^^^^^^^^^^^^^

  Description
     Removes all messages from a queue. Valid arguments are: mode, server, help, or all.

  Returns
     the number of deleted lines from the specified queue.


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set removed [clearqueue help]

----

.. _tcl-cap:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
cap <ls/values/req/enabled/raw> [arg]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Displays CAP status or sends a raw CAP command to the server. ``ls`` will list the capabilities Eggdrop is internally tracking as supported by the server. ``values`` will list all capabilities and their associated CAP 302 values (if any) as a key/value pair, and ``values`` with a capability name as arg will list the values associated for the capability. ``enabled`` will list the capabilities Eggdrop is internally tracking as negotiated with the server. ``req`` will request the capabilities listed in ``arg`` from the server. ``raw`` will send a raw CAP command to the server. The arg field is a single argument, and should be submitted as a single string. For example, to request capabilities foo and bar, you would use [cap req "foo bar"], and for example purposes, sending the same request as a raw command would be [cap raw ``REQ :foo bar``].

  Returns
     a list of CAP capabilities for the ``enabled`` and ``ls`` sub-commands; a dict of capability/value pairs for the ``values`` command or a list if ``values`` if followed by an argument; otherwise nothing.


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set capabilities [cap ls]

----

.. _tcl-tagmsg:

^^^^^^^^^^^^^^^^^^^^^^
tagmsg <tags> <target>
^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends an IRCv3 TAGMSG command to the target. Only works if message-tags has been negotiated with the server via the cap command. tags is a Tcl dict (or space-separated string) of the tags you wish to send separated by commas (do not include the @prefix), and target is the nickname or channel you wish to send the tags to. To send a tag only (not a key/value pair), use a ``""`` as the value for a key in a dict, or a ``{}`` if you are sending as a space-separated string.


  Returns
     nothing


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set tags [dict create +example value]
        tagmsg $tags "#lamest"

----

.. _tcl-server-add:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
server add <ip/host> [[+]port [password]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a server to the list of servers Eggdrop will connect to. Prefix the port with ``+`` to indicate an SSL-protected port. A port value is required if password is to be specified. The SSL status (+) of the provided port is matched against as well (ie, 7000 is not the same as +7000).

  Returns
     nothing


  Module
     ``server``


  .. admonition:: Example — Add a server
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        server add irc.example.net +6697

----

.. _tcl-server-remove:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
server remove <ip/host> [[+]port]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes a server from the list of servers Eggdrop will connect to. If no port is provided, all servers matching the ip or hostname provided will be removed, otherwise only the ip/host with the corresponding port will be removed. The SSL status (+) of the provided port is matched against as well (ie, 7000 is not the same as +7000).

  Returns
     nothing


  Module
     ``server``


  .. admonition:: Example — Remove a server
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        server remove irc.example.net +6697

----

.. _tcl-server-list:

^^^^^^^^^^^
server list
^^^^^^^^^^^

  Description
     Lists all servers currently added to the bots internal server list.

  Returns
     A list of lists in the format ``{{hostname} {port} {password}}``


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set servers [server list]

User Record Manipulation Commands
---------------------------------

.. _tcl-countusers:

^^^^^^^^^^
countusers
^^^^^^^^^^

  Description
     Returns the number of users in the bot's user database.


  Returns
     number of users in the bot's database


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set usercount [countusers]
        putlog "LamestBot has $usercount users."

----

.. _tcl-validuser:

^^^^^^^^^^^^^^^^^^
validuser <handle>
^^^^^^^^^^^^^^^^^^

  Description
     Checks whether a user with the specified handle exists in the bot's user database.


  Returns
     ``1`` if a user by that name exists; ``0`` otherwise


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        if {[validuser foobar]} {
            putlog "The handle foobar exists."
        }

----

.. _tcl-finduser:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
finduser [-account] <value>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Finds the internal user record which most closely matches the given value. When used with the -account flag, value is a services account name, otherwise by default value is a string in the hostmask format of nick!user\@host.

  Returns
     the handle found, or ``*`` if none


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set handle [finduser "Foobar!foobar@127.0.0.1"]

----

.. _tcl-userlist:

^^^^^^^^^^^^^^^^
userlist [flags]
^^^^^^^^^^^^^^^^

  Description
     Returns users from the bot's user database, optionally filtered by a flag mask.


  Returns
     a list of users on the bot. You can use the flag matching system here ([global]``{&/\|}``[chan]``{&/\|}``[bot]). '&' specifies ``and``; '|' specifies ``or``.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set users [userlist]

----

.. _tcl-passwdok:

^^^^^^^^^^^^^^^^^^^^^^^^
passwdok <handle> <pass>
^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Checks the password given against the user's password. Check against the password ``-`` to find out if a user has no password set.

  Returns
     ``1`` if the password matches for that user; ``0`` otherwise. Or if we are checking against the password ``-``: ``1`` if the user has no password set; ``0`` otherwise.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set valid [passwdok foobar "example-password"]

----

.. _tcl-getuser:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getuser <handle> [entry-type] [extra info]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     An interface to the new generic userfile support. Without an entry-type, it returns a flat key/value list (dict) of all set entries. Valid entry types are:

     .. list-table::
        :widths: 11 89
        :header-rows: 1

        * - ``entry-type``
          - Description
        * - ``ACCOUNT``
          - returns the list of service accounts associated with the user
        * - ``BOTFL``
          - returns the current bot-specific flags for the user (bot-only)
        * - ``BOTADDR``
          - returns a list containing the bot's address, bot listen port, and user listen port
        * - ``HOSTS``
          - returns a list of hosts for the user
        * - ``LASTON``
          - returns a list containing the unixtime last seen and the last seen place.
            LASTON #channel returns the time last seen time for the channel or 0 if no info
            exists.
        * - ``INFO``
          - returns the user's global info line
        * - ``XTRA``
          - returns the user's XTRA info
        * - ``COMMENT``
          - returns the master-visible only comment for the user
        * - ``HANDLE``
          - returns the user's handle as it is saved in the userfile
        * - ``PASS``
          - returns the user's encrypted password

     For additional custom user fields, to include the deprecated ``EMAIL`` and ``URL`` fields, reference ``scripts/userinfo.tcl.``

  Returns
     info specific to each entry-type


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set hosts [getuser foobar HOSTS]

----

.. _tcl-setuser:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setuser <handle> <entry-type> [extra info]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     This is the counterpart of getuser. It lets you set the various values. Other then the ones listed below, the entry-types are the same as getuser's.

     .. list-table::
        :widths: 9 91
        :header-rows: 1

        * - ``entry-type``
          - Description
        * - ``ACCOUNT``
          - [account]
            If no value is specified, all accounts for the user will be cleared. Otherwise, only
            a single account will be added to the account list
        * - ``PASS``
          - ``<password>``
            Password string (Empty value will clear the password)
        * - ``BOTADDR``
          - ``<address>`` [bot listen port] [user listen port]
            Sets address, bot listen port and user listen port. If no listen ports are
            specified, only the bot address is updated. If only the bot listen port is
            specified, both the bot and user listen ports are set to the bot listen port.
        * - ``HOSTS``
          - [hostmask]
            If no value is specified, all hosts for the user will be cleared. Otherwise, only
            *1* hostmask is added :P
        * - ``LASTON``
          - This setting has 3 forms.

            ``<unixtime>`` ``<place>``
            sets global LASTON time. Standard values used by Eggdrop for ``<place>`` are partyline,
            linked, unlinked, filearea, <#channel>, and <@remotebotname>, but can be set to
            anything.

            ``<unixtime>``
            sets global LASTON time (leaving the place field empty)

            ``<unixtime>`` ``<channel>``
            sets a user's LASTON time for a channel (if it is a valid channel)

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setuser foobar INFO "Example user information"

----

.. _tcl-chhandle:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chhandle <old-handle> <new-handle>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Changes a user's handle.

  Returns
     ``1`` on success; ``0`` if the new handle is invalid or already used, or if the user can't be found


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set changed [chhandle foobar foobar2]

----

.. _tcl-chattr:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chattr <handle> [changes [channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Changes the attributes for a user record, if you include any.
     Changes are of the form ``+f``, ``-o``, ``+dk``, ``-o+d``, etc. If changes are specified in the format of \|``<changes>`` ``<channel>``, the channel-specific flags for that channel are altered. You can now use the +o|-o #channel format here too.

  Returns
     new flags for the user (if you made no changes, the current flags are returned). If a channel was specified, the global AND the channel-specific flags for that channel are returned in the format of globalflags|channelflags. ``*`` is returned if the specified user does not exist.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set flags [chattr foobar +o "#lamest"]

----

.. _tcl-botattr:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
botattr <handle> [changes [channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Similar to chattr except this modifies bot flags rather than normal user attributes.

  Returns
     new flags for the bot (if you made no changes, the current flags are returned). If a channel was specified, the global AND the channel-specific flags for that channel are returned in the format of globalflags|channelflags. ``*`` is returned if the specified bot does not exist.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set flags [botattr OtherBot]

.. _matchattr:

----

.. _tcl-matchattr:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchattr <handle> <flags> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Checks if the flags of the specified user match the flags provided. ``flags`` is of the form::

         [+/-]<global flags>[&/|<channel flags>[&/|<bot flags>]]

     Either | or & can be used as a separator between global, channel, and bot flags, but only one separator can be used per flag section. A ``+`` is used to check if a user has the subsequent flags, and a ``-`` is used to check if a user does NOT have the subsequent flags. Please see `Flag Masks`_ for additional information on flag usage.

  Returns
     ``1`` if the specified user has the flags matching the provided mask; ``0`` otherwise


  Module
     ``core``


  .. admonition:: Example — Check user flags
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        if {[matchattr foobar o "#lamest"]} {
            putlog "foobar has the requested flags."
        }

----

.. _tcl-adduser:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
adduser <handle> [hostmask]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Creates a new user entry with the handle and hostmask given (with no password and the default flags).

  Returns
     ``1`` if successful; ``0`` if the handle already exists


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set added [adduser foobar "Foobar!foobar@127.0.0.1"]

----

.. _tcl-addbot:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
addbot <handle> <address> [botport [userport]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a new bot to the userlist with the handle and botaddress given (with no password and no flags). ``<address>`` format is one of:

     - ipaddress
     - ipv4address:botport/userport    [DEPRECATED]
     - [ipv6address]:botport/userport  [DEPRECATED]

NOTE 1: The []s around the ipv6address argument are literal []s, not optional arguments.
NOTE 2: In the deprecated formats, an additional botport and/or userport given as follow-on arguments are ignored.

  Returns
     ``1`` if successful; ``0`` if the bot already exists or a port is invalid


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set added [addbot OtherBot 127.0.0.1]

----

.. _tcl-deluser:

^^^^^^^^^^^^^^^^
deluser <handle>
^^^^^^^^^^^^^^^^

  Description
     Attempts to erase the user record for a handle.

  Returns
     ``1`` if successful, ``0`` if no such user exists


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set deleted [deluser foobar]

----

.. _tcl-delhost:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
delhost <handle> <hostmask>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Deletes a hostmask from a user's host list.

  Returns
     ``1`` on success; ``0`` if the hostmask (or user) doesn't exist


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [delhost foobar "Foobar!foobar@127.0.0.1"]

----

.. _tcl-addchanrec:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
addchanrec <handle> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a channel record for a user.

  Returns
     ``1`` on success; ``0`` if the user or channel does not exist


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set added [addchanrec foobar "#lamest"]

----

.. _tcl-delchanrec:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
delchanrec <handle> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes a channel record for a user. This includes all associated channel flags.

  Returns
     ``1`` on success; ``0`` if the user or channel does not exist


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set deleted [delchanrec foobar "#lamest"]

----

.. _tcl-haschanrec:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
haschanrec <handle> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Checks whether a user has a channel record for the specified channel.


  Returns
     ``1`` if the given handle has a chanrec for the specified channel; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set has_record [haschanrec foobar "#lamest"]

----

.. _tcl-getchaninfo:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getchaninfo <handle> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a user's info line for the specified channel.


  Returns
     info line for a specific channel (behaves just like 'getinfo')


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set info [getchaninfo foobar "#lamest"]

----

.. _tcl-setchaninfo:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setchaninfo <handle> <channel> <info>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sets the info line on a specific channel for a user. If info is ``none``, it will be removed.

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setchaninfo foobar "#lamest" "Example channel information"

----

.. _tcl-newchanban:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newchanban <channel> <ban> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a ban to the ban list of a channel; creator is given credit for the ban in the ban list. lifetime is specified in minutes. If lifetime is not specified, ban-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent ban.

  Options

     .. list-table::
        :widths: 11 89
        :header-rows: 1

        * - ``options``
          - Description
        * - ``sticky``
          - forces the ban to be always active on a channel, even with dynamicbans on


  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example — Add a channel ban
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        newchanban "#lamest" "*!*@127.0.0.1" foobar "Example text"

----

.. _tcl-newban:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newban <ban> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a ban to the global ban list (which takes effect on all channels); creator is given credit for the ban in the ban list. lifetime is specified in minutes. If lifetime is not specified, default-ban-time (usually 120) is used. Setting the lifetime to 0 makes it a permanent ban.

  Options

     .. list-table::
        :widths: 11 89
        :header-rows: 1

        * - ``options``
          - Description
        * - ``sticky``
          - forces the ban to be always active on a channel, even with dynamicbans on

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        newban "*!*@127.0.0.1" foobar "Example text"

----

.. _tcl-newchanexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newchanexempt <channel> <exempt> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a exempt to the exempt list of a channel; creator is given credit for the exempt in the exempt list. lifetime is specified in minutes. If lifetime is not specified, exempt-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent exempt. The exempt will not be removed until the corresponding ban has been removed. For timed bans, once the time period has expired, the exempt will not be removed until the corresponding ban has either expired or been removed.

  Options

     .. list-table::
        :widths: 11 89
        :header-rows: 1

        * - ``options``
          - Description
        * - ``sticky``
          - forces the exempt to be always active on a channel, even with dynamicexempts on

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        newchanexempt "#lamest" "*!*@127.0.0.1" foobar "Example text"

----

.. _tcl-newexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newexempt <exempt> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a exempt to the global exempt list (which takes effect on all channels); creator is given credit for the exempt in the exempt list. lifetime is specified in minutes. If lifetime is not specified, exempt-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent exempt. The exempt will not be removed until the corresponding ban has been removed.

  Options

     .. list-table::
        :widths: 11 89
        :header-rows: 1

        * - ``options``
          - Description
        * - ``sticky``
          - forces the exempt to be always active on a channel, even with dynamicexempts on

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        newexempt "*!*@127.0.0.1" foobar "Example text"

----

.. _tcl-newchaninvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newchaninvite <channel> <invite> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a invite to the invite list of a channel; creator is given credit for the invite in the invite list. lifetime is specified in minutes. If lifetime is not specified, invite-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent invite. The invite will not be removed until the channel has gone -i.

  Options

     .. list-table::
        :widths: 11 89
        :header-rows: 1

        * - ``options``
          - Description
        * - ``sticky``
          - forces the invite to be always active on a channel, even with dynamicinvites on

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        newchaninvite "#lamest" "*!*@127.0.0.1" foobar "Example text"

----

.. _tcl-newinvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newinvite <invite> <creator> <comment> [lifetime] [options]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a invite to the global invite list (which takes effect on all channels); creator is given credit for the invite in the invite list. lifetime is specified in minutes. If lifetime is not specified, invite-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent invite. The invite will not be removed until the channel has gone -i.

  Options

     .. list-table::
        :widths: 11 89
        :header-rows: 1

        * - ``options``
          - Description
        * - ``sticky``
          - forces the invite to be always active on a channel, even with dynamicinvites on

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        newinvite "*!*@127.0.0.1" foobar "Example text"

----

.. _tcl-stickban:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
stickban <banmask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Makes a ban sticky, or, if a channel is specified, then it is set sticky on that channel only.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [stickban "*!*@127.0.0.1"]

----

.. _tcl-unstickban:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
unstickban <banmask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Makes a ban no longer sticky, or, if a channel is specified, then it is unstuck on that channel only.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [unstickban "*!*@127.0.0.1"]

----

.. _tcl-stickexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
stickexempt <exemptmask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Makes an exempt sticky, or, if a channel is specified, then it is set sticky on that channel only.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [stickexempt "*!*@127.0.0.1"]

----

.. _tcl-unstickexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
unstickexempt <exemptmask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Makes an exempt no longer sticky, or, if a channel is specified, then it is unstuck on that channel only.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [unstickexempt "*!*@127.0.0.1"]

----

.. _tcl-stickinvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
stickinvite <invitemask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Makes an invite sticky, or, if a channel is specified, then it is set sticky on that channel only.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [stickinvite "*!*@127.0.0.1"]

----

.. _tcl-unstickinvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
unstickinvite <invitemask> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Makes an invite no longer sticky, or, if a channel is specified, then it is unstuck on that channel only.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [unstickinvite "*!*@127.0.0.1"]

----

.. _tcl-killchanban:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
killchanban <channel> <ban>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes a ban from the ban list for a channel.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [killchanban "#lamest" "*!*@127.0.0.1"]

----

.. _tcl-killban:

^^^^^^^^^^^^^
killban <ban>
^^^^^^^^^^^^^

  Description
     Removes a ban from the global ban list.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [killban "*!*@127.0.0.1"]

----

.. _tcl-killchanexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
killchanexempt <channel> <exempt>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes an exempt from the exempt list for a channel.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [killchanexempt "#lamest" "*!*@127.0.0.1"]

----

.. _tcl-killexempt:

^^^^^^^^^^^^^^^^^^^
killexempt <exempt>
^^^^^^^^^^^^^^^^^^^

  Description
     Removes an exempt from the global exempt list.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [killexempt "*!*@127.0.0.1"]

----

.. _tcl-killchaninvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
killchaninvite <channel> <invite>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes an invite from the invite list for a channel.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [killchaninvite "#lamest" "*!*@127.0.0.1"]

----

.. _tcl-killinvite:

^^^^^^^^^^^^^^^^^^^
killinvite <invite>
^^^^^^^^^^^^^^^^^^^

  Description
     Removes an invite from the global invite list.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [killinvite "*!*@127.0.0.1"]

----

.. _tcl-ischanjuped:

^^^^^^^^^^^^^^^^^^^^^
ischanjuped <channel>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the channel is juped, and the bot is unable to join; 0 otherwise.

  Returns
     ``1`` if the channel is juped, and the bot is unable to join; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set juped [ischanjuped "#lamest"]

----

.. _tcl-isban:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isban <ban> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified ban is in the global ban list; 0 otherwise. If a channel is specified, that channel's ban list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel bans are checked.

  Returns
     ``1`` if the specified ban is in the global ban list; ``0`` otherwise. If a channel is specified, that channel's ban list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel bans are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [isban "*!*@127.0.0.1"]

----

.. _tcl-ispermban:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ispermban <ban> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified ban is in the global ban list AND is marked as permanent; 0 otherwise. If a channel is specified, that channel's ban list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel bans are checked.

  Returns
     ``1`` if the specified ban is in the global ban list AND is marked as permanent; ``0`` otherwise. If a channel is specified, that channel's ban list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel bans are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [ispermban "*!*@127.0.0.1"]

----

.. _tcl-isexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isexempt <exempt> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified exempt is in the global exempt list; 0 otherwise. If a channel is specified, that channel's exempt list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel exempts are checked.

  Returns
     ``1`` if the specified exempt is in the global exempt list; ``0`` otherwise. If a channel is specified, that channel's exempt list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel exempts are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [isexempt "*!*@127.0.0.1"]

----

.. _tcl-ispermexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ispermexempt <exempt> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified exempt is in the global exempt list AND is marked as permanent; 0 otherwise. If a channel is specified, that channel's exempt list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel exempts are checked.

  Returns
     ``1`` if the specified exempt is in the global exempt list AND is marked as permanent; ``0`` otherwise. If a channel is specified, that channel's exempt list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel exempts are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [ispermexempt "*!*@127.0.0.1"]

----

.. _tcl-isinvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isinvite <invite> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified invite is in the global invite list; 0 otherwise. If a channel is specified, that channel's invite list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel invites are checked.

  Returns
     ``1`` if the specified invite is in the global invite list; ``0`` otherwise. If a channel is specified, that channel's invite list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel invites are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [isinvite "*!*@127.0.0.1"]

----

.. _tcl-isperminvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isperminvite <invite> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified invite is in the global invite list AND is marked as permanent; 0 otherwise. If a channel is specified, that channel's invite list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel invites are checked.

  Returns
     ``1`` if the specified invite is in the global invite list AND is marked as permanent; ``0`` otherwise. If a channel is specified, that channel's invite list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel invites are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [isperminvite "*!*@127.0.0.1"]

----

.. _tcl-isbansticky:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isbansticky <ban> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified ban is marked as sticky in the global ban list; 0 otherwise. If a channel is specified, that channel's ban list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel bans are checked.

  Returns
     ``1`` if the specified ban is marked as sticky in the global ban list; ``0`` otherwise. If a channel is specified, that channel's ban list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel bans are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [isbansticky "*!*@127.0.0.1"]

----

.. _tcl-isexemptsticky:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isexemptsticky <exempt> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified exempt is marked as sticky in the global exempt list; 0 otherwise. If a channel is specified, that channel's exempt list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel exempts are checked.

  Returns
     ``1`` if the specified exempt is marked as sticky in the global exempt list; ``0`` otherwise. If a channel is specified, that channel's exempt list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel exempts are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [isexemptsticky "*!*@127.0.0.1"]

----

.. _tcl-isinvitesticky:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isinvitesticky <invite> [channel [-channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified invite is marked as sticky in the global invite list; 0 otherwise. If a channel is specified, that channel's invite list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel invites are checked.

  Returns
     ``1`` if the specified invite is marked as sticky in the global invite list; ``0`` otherwise. If a channel is specified, that channel's invite list is checked as well. If the -channel flag is used at the end of the command, \*only\* the channel invites are checked.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [isinvitesticky "*!*@127.0.0.1"]

----

.. _tcl-matchban:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchban <nick!user@host> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified nick!user\@host matches a ban in the global ban list; 0 otherwise. If a channel is specified, that channel's ban list is checked as well.

  Returns
     ``1`` if the specified nick!user\@host matches a ban in the global ban list; ``0`` otherwise. If a channel is specified, that channel's ban list is checked as well.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set banned [matchban "Foobar!foobar@127.0.0.1" "#lamest"]

----

.. _tcl-matchexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchexempt <nick!user@host> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified nick!user\@host matches an exempt in the global exempt list; 0 otherwise. If a channel is specified, that channel's exempt list is checked as well.

  Returns
     ``1`` if the specified nick!user\@host matches an exempt in the global exempt list; ``0`` otherwise. If a channel is specified, that channel's exempt list is checked as well.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set exempt [matchexempt "Foobar!foobar@127.0.0.1" "#lamest"]

----

.. _tcl-matchinvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchinvite <nick!user@host> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified nick!user\@host matches an invite in the global invite list; 0 otherwise. If a channel is specified, that.

  Returns
     ``1`` if the specified nick!user\@host matches an invite in the global invite list; ``0`` otherwise. If a channel is specified, that
     channel's invite list is checked as well.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set invited [matchinvite "Foobar!foobar@127.0.0.1" "#lamest"]

----

.. _tcl-banlist:

^^^^^^^^^^^^^^^^^
banlist [channel]
^^^^^^^^^^^^^^^^^

  Description
     Returns a list of global bans, or, if a channel is specified, a list of channel-specific bans. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.

  Returns
     a list of global bans, or, if a channel is specified, a list of channel-specific bans. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set bans [banlist "#lamest"]

----

.. _tcl-exemptlist:

^^^^^^^^^^^^^^^^^^^^
exemptlist [channel]
^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a list of global exempts, or, if a channel is specified, a list of channel-specific exempts. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.

  Returns
     a list of global exempts, or, if a channel is specified, a list of channel-specific exempts. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set exempts [exemptlist "#lamest"]

----

.. _tcl-invitelist:

^^^^^^^^^^^^^^^^^^^^
invitelist [channel]
^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a list of global invites, or, if a channel is specified, a list of channel-specific invites. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.

  Returns
     a list of global invites, or, if a channel is specified, a list of channel-specific invites. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, last time active, and creator. The three timestamps are in unixtime format.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set invites [invitelist "#lamest"]

----

.. _tcl-newignore:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
newignore <hostmask> <creator> <comment> [lifetime]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds an entry to the ignore list; creator is given credit for the ignore. lifetime is how many minutes until the ignore expires and is removed. If lifetime is not specified, ignore-time (usually 60) is used. Setting the lifetime to 0 makes it a permanent ignore.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        newignore "*!*@127.0.0.1" foobar "Example ignore" 60

----

.. _tcl-killignore:

^^^^^^^^^^^^^^^^^^^^^
killignore <hostmask>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes an entry from the ignore list.

  Returns
     ``1`` if successful; ``0`` otherwise


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set removed [killignore "*!*@127.0.0.1"]

----

.. _tcl-ignorelist:

^^^^^^^^^^
ignorelist
^^^^^^^^^^

  Description
     Returns a list of ignores. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, and creator. The timestamps are in unixtime format.

  Returns
     a list of ignores. Each entry is a sublist containing: hostmask, comment, expiration timestamp, time added, and creator. The timestamps are in unixtime format.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set ignores [ignorelist]

----

.. _tcl-isignore:

^^^^^^^^^^^^^^^^^^^
isignore <hostmask>
^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the ignore is in the list; 0 otherwise.

  Returns
     ``1`` if the ignore is in the list; ``0`` otherwise


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set ignored [isignore "*!*@127.0.0.1"]

----

.. _tcl-save:

^^^^
save
^^^^

  Description
     Writes the user and channel files to disk.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        save

----

.. _tcl-reload:

^^^^^^
reload
^^^^^^

  Description
     Loads the userfile from disk, replacing whatever is in memory.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        reload

----

.. _tcl-backup:

^^^^^^
backup
^^^^^^

  Description
     Makes a simple backup of the userfile that's on disk. If the channels module is loaded, this also makes a simple backup of the channel file.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        backup

----

.. _tcl-getting-users:

^^^^^^^^^^^^^
getting-users
^^^^^^^^^^^^^

  Description
     Returns ``1`` if the bot is currently downloading a userfile from a sharebot (and hence, user records are about to drastically change); 0 if not.

  Returns
     ``1`` if the bot is currently downloading a userfile from a sharebot (and hence, user records are about to drastically change); ``0`` if not


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [getting-users]

Channel Commands
----------------

.. _tcl-channel-add:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
channel add <name> [option-list]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Adds a channel record for the bot to monitor. The full list of possible options are given in ``doc/settings/mod.channels.`` Note that the channel options must be in a list (enclosed in {}).

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example — Add a channel
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        channel add "#lamest"

----

.. _tcl-channel-set:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
channel set <name> <options...>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sets options for the channel specified. `options` is a flat list of either +/-settings or key/value pairs. The full list of possible options are given in ``doc/settings/mod.channels.`` Note: Tcl code settings such as the need-* settings must be valid Tcl code as a single word, for example ``channel set #lamestst need-op { putmsg ChanServ "op #lamestst" }``.

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example — Change a channel setting
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        channel set "#lamest" +autoop

----

.. _tcl-channel-info:

^^^^^^^^^^^^^^^^^^^
channel info <name>
^^^^^^^^^^^^^^^^^^^

  Description
     Returns the settings currently stored for the specified channel.


  Returns
     a list of info about the specified channel's settings.


  Module
     ``channels``


  .. admonition:: Example — Inspect channel settings
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set info [channel info "#lamest"]

----

.. _tcl-channel-get:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
channel get <name> [setting]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a channel setting, or all channel settings when no setting is specified.


  Returns
     The value of the setting you specify. For flags, a value of ``0`` means it is disabled (-), and non-zero means enabled (+). If no setting is specified, a flat list of all available settings and their values will be returned.


  Module
     ``channels``


  .. admonition:: Example — Read a channel setting
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set autoop [channel get "#lamest" autoop]

----

.. _tcl-channel-remove:

^^^^^^^^^^^^^^^^^^^^^
channel remove <name>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes a channel record from the bot and makes the bot no longer monitor the channel.

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        channel remove "#lamest"

----

.. _tcl-savechannels:

^^^^^^^^^^^^
savechannels
^^^^^^^^^^^^

  Description
     Saves the channel settings to the channel-file if one is defined.

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        savechannels

----

.. _tcl-loadchannels:

^^^^^^^^^^^^
loadchannels
^^^^^^^^^^^^

  Description
     Reloads the channel settings from the channel-file if one is defined.

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        loadchannels

----

.. _tcl-channels:

^^^^^^^^
channels
^^^^^^^^

  Description
     Returns the channels for which the bot has channel records.


  Returns
     a list of the channels the bot has a channel record for


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set channel_list [channels]

----

.. _tcl-channame2dname:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
channame2dname <channel-name>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Converts a real IRC channel name to the channel description name Eggdrop uses internally. This is primarily relevant to !channels; for other channels, the two names are the same.

  Returns
     the channel description name corresponding to <channel-name>

  See also
     :ref:`chandname2name <tcl-chandname2name>`


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set dname [channame2dname "#lamest"]

----

.. _tcl-chandname2name:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chandname2name <channel-dname>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Converts an Eggdrop channel description name to the real IRC channel name used by the server. This is primarily relevant to !channels; for other channels, the two names are the same.

  Returns
     the real IRC channel name corresponding to <channel-dname>

  See also
     :ref:`channame2dname <tcl-channame2dname>`


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set channel_name [chandname2name "#lamest"]

----

.. _tcl-isbotnick:

^^^^^^^^^^^^^^^^
isbotnick <nick>
^^^^^^^^^^^^^^^^

  Description
     Checks whether the specified nickname is the bot's current nickname.


  Returns
     ``1`` if the nick matches the botnick; ``0`` otherwise


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set is_bot [isbotnick LamestBot]

----

.. _tcl-botisop:

^^^^^^^^^^^^^^^^^
botisop [channel]
^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the bot has ops on the specified channel (or any channel if no channel is specified); 0 otherwise.

  Returns
     ``1`` if the bot has ops on the specified channel (or any channel if no channel is specified); ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set opped [botisop "#lamest"]

----

.. _tcl-botishalfop:

^^^^^^^^^^^^^^^^^^^^^
botishalfop [channel]
^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the bot has halfops on the specified channel (or any channel if no channel is specified); 0 otherwise.

  Returns
     ``1`` if the bot has halfops on the specified channel (or any channel if no channel is specified); ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set halfopped [botishalfop "#lamest"]

----

.. _tcl-botisvoice:

^^^^^^^^^^^^^^^^^^^^
botisvoice [channel]
^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the bot has a voice on the specified channel (or any channel if no channel is specified); 0 otherwise.

  Returns
     ``1`` if the bot has a voice on the specified channel (or any channel if no channel is specified); ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set voiced [botisvoice "#lamest"]

----

.. _tcl-botonchan:

^^^^^^^^^^^^^^^^^^^
botonchan [channel]
^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the bot is on the specified channel (or any channel if no channel is specified); 0 otherwise.

  Returns
     ``1`` if the bot is on the specified channel (or any channel if no channel is specified); ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set present [botonchan "#lamest"]

----

.. _tcl-isop:

^^^^^^^^^^^^^^^^^^^^^^^^^
isop <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if someone by the specified nickname is on the channel (or any channel if no channel name is specified) and has ops; 0 otherwise.

  Returns
     ``1`` if someone by the specified nickname is on the channel (or any channel if no channel name is specified) and has ops; ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set opped [isop Foobar "#lamest"]

----

.. _tcl-ishalfop:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ishalfop <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if someone by the specified nickname is on the channel (or any channel if no channel name is specified) and has halfops; 0 otherwise.

  Returns
     ``1`` if someone by the specified nickname is on the channel (or any channel if no channel name is specified) and has halfops; ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set halfopped [ishalfop Foobar "#lamest"]

----

.. _tcl-wasop:

^^^^^^^^^^^^^^^^^^^^^^^^^^
wasop <nickname> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if someone that just got opped/deopped in the chan had op before the modechange; 0 otherwise.

  Returns
     ``1`` if someone that just got opped/deopped in the chan had op before the modechange; ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set previously_opped [wasop Foobar "#lamest"]

----

.. _tcl-washalfop:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
washalfop <nickname> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if someone that just got halfopped/dehalfopped in the chan had halfop before the modechange; 0 otherwise.

  Returns
     ``1`` if someone that just got halfopped/dehalfopped in the chan had halfop before the modechange; ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set previously_halfopped [washalfop Foobar "#lamest"]

----

.. _tcl-isvoice:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isvoice <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if someone by that nickname is on the channel (or any channel if no channel is specified) and has voice (+v); 0 otherwise.

  Returns
     ``1`` if someone by that nickname is on the channel (or any channel if no channel is specified) and has voice (+v); ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set voiced [isvoice Foobar "#lamest"]

----

.. _tcl-isidentified:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isidentified <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Determine if a user is identified to irc services. WARNING: this may not be accurate depending on the server and configuration. For accurate results, the server must support (and Eggdrop must have enabled via CAP) the account-notify and extended-join capabilities, and the server must understand WHOX requests (also known as raw 354 responses).

  Returns
     ``1`` if someone by the specified nickname is on the channel (or any channel if no channel name is specified) and is logged in); ``0`` otherwise.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set identified [isidentified Foobar "#lamest"]

----

.. _tcl-isaway:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
isaway <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Determine if a user is marked as ``away`` on a server. IMPORTANT: this command is only ``mostly`` reliable on its own when the IRCv3 away-notify capability is available and negotiated with the IRC server (if you didn't add this to your config file, it likely isn't enabled- you can confirm using the ``cap`` Tcl command). Additionally, there is no way for Eggdrop (or any client) to capture a user's away status when the user first joins a channel (they are assumed present by Eggdrop on join). To use this command without the away-notify capability negotiated, or to get a user's away status on join (via a JOIN bind), use ``refreshchan <channel> w`` on a channel the user is on, which will refresh the current away status stored by Eggdrop for all users on the channel.

  Returns
     ``1`` if Eggdrop is currently tracking someone by that nickname marked as 'away' (again, see disclaimer above) by an IRC server; ``0`` otherwise.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set away [isaway Foobar "#lamest"]

----

.. _tcl-isircbot:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
isircbot <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Determine if a user has denoted themselves as a bot via an ircd-defined user flag (declared via BOT in a server's 005/ISUPPORT line). Due to server implementations, accurately monitoring this is incredibly fragile, as the flag can be added and removed by a user without any notification to other users. To ensure this status is current for use, it is recommended to use ``refreshchan <channel> w`` on a channel the user is on, which will refresh if the user is a bot or not for all users on the channel. If a server does not advertise BOT in its ISUPPORT line but still supports it (currently the case for unrealircd), you can manually set it by adding ``BOT=B`` (or whatever flag is used) to the isupport-default setting in your eggdrop.conf file.

  Returns
     ``1`` if Eggdrop is currently tracking someone by that nickname marked as a bot by an IRC server; ``0`` otherwise.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set is_bot [isircbot Foobar "#lamest"]

----

.. _tcl-onchan:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
onchan <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if someone by that nickname is on the specified channel (or any channel if none is specified); 0 otherwise.
  Returns
     ``1`` if someone by that nickname is on the specified channel (or any channel if none is specified); ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set present [onchan Foobar "#lamest"]

----

.. _tcl-monitor:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
monitor <add/delete/list/online/offline/status/clear> [nickname]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Interacts with the list of nicknames Eggdrop has asked the IRC server to track. valid sub-commands are add, delete, list, online, offline, status, and clear. The ``add`` command sends ``nickname`` to the server to track. The ``delete`` command removes ``nickname`` from being tracked by the server (or returns an error if the nickname is not present). The ``list`` command returns a list of all nicknames the IRC server is tracking on behalf of Eggdrop. The ``online`` command returns a string of tracked nicknames that are currently online. The ``offline`` command returns a list of tracked nicknames that are currently offline.

  Returns
     The ``add`` sub-command returns a ``1`` if the nick was successfully added, a ``0`` if the nick is already in the monitor list, and a ``2`` if the nick could not be added. The ``delete`` sub-command returns a ``1`` if the nick is removed, or an error if the nick is not found. The ``status`` sub-command returns a ``1`` if ``nickname`` is online or a ``0`` if ``nickname`` is offline. The ``clear`` command removes all nicknames from the list the server is monitoring.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set added [monitor add Foobar]

----

.. _tcl-accounttracking:

^^^^^^^^^^^^^^^
accounttracking
^^^^^^^^^^^^^^^

  Description
     Checks to see if the three required functionalities to enable proper account tracking are available (and enabled) to Eggdrop. This checks if the extended-join and account-notify IRCv3 capabilities are currently enabled, and checks if the server supports WHOX (based on the type of server selected in the config file, or the use-354 variable being set to 1 when selecting an "Other" server).

  Returns
     a ``1`` if all three functionalities are present, a ``0`` if one or more are missing.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [accounttracking]

----

.. _tcl-getaccount:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getaccount <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns the services account name associated with nickname, ``*`` if the user is not logged into services, or ``""`` if eggdrop does not know the account status of the user.

  Returns
     the services account name associated with nickname, ``*`` if the user is not logged into services, or ``""`` if eggdrop does not know the account status of the user.

     NOTE: the three required IRC components for account tracking are: the WHOX feature, the extended-join IRCv3 capability and the account-notify IRCv3 capability. if only some of the three feature are available, eggdrop provides best-effort account tracking. please see doc/ACCOUNTS for additional information.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set account [getaccount Foobar "#lamest"]

----

.. _tcl-nick2hand:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
nick2hand <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns the handle of a nickname on a channel. If a channel is not specified, the bot will check all of its channels. If the nick is not found, ``""`` is returned. If the nick is found but does not have a handle, ``*`` is returned. If no channel is specified, all channels are checked.

  Returns
     the handle of a nickname on a channel. If a channel is not specified, the bot will check all of its channels. If the nick is not found, ``""`` is returned. If the nick is found but does not have a handle, ``*`` is returned. If no channel is specified, all channels are checked.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set handle [nick2hand Foobar "#lamest"]

----

.. _tcl-account2nicks:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
account2nicks <account> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a de-duplicated Tcl list of the nickname(s) on the specified channel (if one is specified) whose nickname matches the given account; ``""`` is returned if no match is found. This command will only work if a server supports (and Eggdrop has enabled) the account-notify and extended-join capabilities, and the server understands WHOX requests (also known as raw 354 responses). If no channel is specified, all channels are checked.

  Returns
     a de-duplicated Tcl list of the nickname(s) on the specified channel (if one is specified) whose nickname matches the given account; ``""`` is returned if no match is found. This command will only work if a server supports (and Eggdrop has enabled) the account-notify and extended-join capabilities, and the server understands WHOX requests (also known as raw 354 responses). If no channel is specified, all channels are checked.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set nicks [account2nicks foobar "#lamest"]

----

.. _tcl-hand2nick:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
hand2nick <handle> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns nickname of the first person on the specified channel (if one is specified) whose nick!user\@host matches the given handle; ``""`` is returned if no match is found. If no channel is specified, all channels are checked.

  Returns
     nickname of the first person on the specified channel (if one is specified) whose nick!user\@host matches the given handle; ``""`` is returned if no match is found. If no channel is specified, all channels are checked.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set nick [hand2nick foobar "#lamest"]

----

.. _tcl-hand2nicks:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
hand2nicks <handle> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a de-duplicated Tcl list of the nickname(s) on the specified channel (if one is specified) whose nick!user\@host matches the given handle; ``""`` is returned if no match is found. If no channel is specified, all channels are checked.

  Returns
     a de-duplicated Tcl list of the nickname(s) on the specified channel (if one is specified) whose nick!user\@host matches the given handle; ``""`` is returned if no match is found. If no channel is specified, all channels are checked.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set nicks [hand2nicks foobar "#lamest"]

----

.. _tcl-handonchan:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
handonchan <handle> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the the nick!user\@host for someone on the channel (or any channel if no channel name is specified) matches for the handle given; 0 otherwise.

  Returns
     ``1`` if the the nick!user\@host for someone on the channel (or any channel if no channel name is specified) matches for the handle given; ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set present [handonchan foobar "#lamest"]

----

.. _tcl-ischanban:

^^^^^^^^^^^^^^^^^^^^^^^^^
ischanban <ban> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified ban is on the given channel's ban list (not the bot's banlist for the channel).

  Returns
     ``1`` if the specified ban is on the given channel's ban list (not the bot's banlist for the channel)


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [ischanban "*!*@127.0.0.1" "#lamest"]

----

.. _tcl-ischanexempt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ischanexempt <exempt> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified exempt is on the given channel's exempt list (not the bot's exemptlist for the channel).

  Returns
     ``1`` if the specified exempt is on the given channel's exempt list (not the bot's exemptlist for the channel)


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [ischanexempt "*!*@127.0.0.1" "#lamest"]

----

.. _tcl-ischaninvite:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ischaninvite <invite> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the specified invite is on the given channel's invite list (not the bot's invitelist for the channel).

  Returns
     ``1`` if the specified invite is on the given channel's invite list (not the bot's invitelist for the channel)


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [ischaninvite "*!*@127.0.0.1" "#lamest"]

----

.. _tcl-chanbans:

^^^^^^^^^^^^^^^^^^
chanbans <channel>
^^^^^^^^^^^^^^^^^^

  Description
     Returns a list of the current bans on the channel. Each element is a sublist of the form {``<ban>`` ``<bywho>`` ``<age>``}. age is seconds from the bot's point of view.

  Returns
     a list of the current bans on the channel. Each element is a sublist of the form ``{<ban> <bywho> <age>}``. age is seconds from the bot's point of view


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set bans [chanbans "#lamest"]

----

.. _tcl-chanexempts:

^^^^^^^^^^^^^^^^^^^^^
chanexempts <channel>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a list of the current exempts on the channel. Each element is a sublist of the form {``<exempts>`` ``<bywho>`` ``<age>``}. age is seconds from the bot's point of view.

  Returns
     a list of the current exempts on the channel. Each element is a sublist of the form ``{<exempts> <bywho> <age>}``. age is seconds from the bot's point of view


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set exempts [chanexempts "#lamest"]

----

.. _tcl-chaninvites:

^^^^^^^^^^^^^^^^^^^^^
chaninvites <channel>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a list of the current invites on the channel. Each element is a sublist of the form {``<invites>`` ``<bywho>`` ``<age>``}. age is seconds from the bot's point of view.

  Returns
     a list of the current invites on the channel. Each element is a sublist of the form ``{<invites> <bywho> <age>}``. age is seconds from the bot's point of view


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set invites [chaninvites "#lamest"]

----

.. _tcl-resetbans:

^^^^^^^^^^^^^^^^^^^
resetbans <channel>
^^^^^^^^^^^^^^^^^^^

  Description
     Removes all bans on the channel that aren't in the bot's ban list and refreshes any bans that should be on the channel but aren't.

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        resetbans "#lamest"

----

.. _tcl-resetexempts:

^^^^^^^^^^^^^^^^^^^^^^
resetexempts <channel>
^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes all exempt on the channel that aren't in the bot's exempt list and refreshes any exempts that should be on the channel but aren't.

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        resetexempts "#lamest"

----

.. _tcl-resetinvites:

^^^^^^^^^^^^^^^^^^^^^^
resetinvites <channel>
^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes all invites on the channel that aren't in the bot's invite list and refreshes any invites that should be on the channel but aren't.

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        resetinvites "#lamest"

----

.. _tcl-resetchanidle:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
resetchanidle [nick] <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Resets the channel idle time for the given nick or for all nicks on the channel if no nick is specified.

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        resetchanidle Foobar "#lamest"

----

.. _tcl-resetchanjoin:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
resetchanjoin [nick] <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Resets the channel join time for the given nick or for all nicks on the channel if no nick is specified.

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        resetchanjoin Foobar "#lamest"

----

.. _tcl-resetchan:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
resetchan <channel> [flags]
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Clears the channel info Eggdrop is currently storing for a channel, then rereads the channel info from the server. Useful if Eggdrop gets into a bad state on a server with respect to a channel userlist, for example. If flags are specified, only the required information will be reset, according to the given flags. Available flags:

     .. list-table::
        :widths: 14 86
        :header-rows: 1

        * - ``flags``
          - Description
        * - ``b``
          - channel bans
        * - ``e``
          - channel exempts
        * - ``I``
          - channel invites
        * - ``m``
          - channel modes
        * - ``w``
          - memberlist (who & away info)

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        resetchan "#lamest" w

----

.. _tcl-refreshchan:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
refreshchan <channel> [flags]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     An alternative to resetchan, refresh rereads the channel info from the server without first clearing out the previously stored information. Useful for updating a user's away status without resetting their idle time, for example. If flags are specified, only the required information will be refreshed, according to the given flags. Available flags:

     .. list-table::
        :widths: 14 86
        :header-rows: 1

        * - ``flags``
          - Description
        * - ``b``
          - channel bans
        * - ``e``
          - channel exempts
        * - ``I``
          - channel invites
        * - ``m``
          - channel modes
        * - ``t``
          - channel topic
        * - ``w``
          - memberlist (who & away info)

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        refreshchan "#lamest" w

----

.. _tcl-getchanhost:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getchanhost <nickname> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns user\@host of the specified nickname (the nickname is not included in the returned host). If a channel is not specified, bot will check all of its channels. If the nickname is not on the channel(s), ``""`` is returned.

  Returns
     user\@host of the specified nickname (the nickname is not included in the returned host). If a channel is not specified, bot will check all of its channels. If the nickname is not on the channel(s), ``""`` is returned.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set host [getchanhost Foobar "#lamest"]

----

.. _tcl-getchanjoin:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getchanjoin <nickname> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns timestamp (unixtime format) of when the specified nickname joined the channel if available, 0 otherwise. Note that after a channel reset this information will be lost, even if previously available.

  Returns
     timestamp (unixtime format) of when the specified nickname joined the channel if available, ``0`` otherwise. Note that after a channel reset this information will be lost, even if previously available.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set joined [getchanjoin Foobar "#lamest"]

----

.. _tcl-onchansplit:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
onchansplit <nick> [channel]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if that nick is split from the channel (or any channel if no channel is specified); 0 otherwise.

  Returns
     ``1`` if that nick is split from the channel (or any channel if no channel is specified); ``0`` otherwise


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set split [onchansplit Foobar "#lamest"]

----

.. _tcl-chanlist:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
chanlist <channel> [flags][<&|>chanflags]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Lists all users on a channel Eggdrop has joined. flags are any global flags; the ``&`` or ``\|`` denotes to look for channel specific flags, where ``&`` will return users having ALL chanflags and ``|`` returns users having ANY of the chanflags (See `Flag Masks`_ for additional information).

  Returns
     Searching for flags optionally preceded with a '+' will return a list of nicknames that have all the flags listed. Searching for flags preceded with a '-' will return a list of nicknames that do not have have any of the flags (differently said, '-' will hide users that have all flags listed). If no flags are given, all of the nicknames on the channel are returned.

     Please note that if you're executing chanlist after a part or sign bind, the gone user will still be listed, so you can check for wasop, isop, etc.


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set nicks [chanlist "#lamest"]

----

.. _tcl-getchanidle:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
getchanidle <nickname> <channel>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns number of minutes that person has been idle; -1 if the specified user isn't on the channel.

  Returns
     number of minutes that person has been idle; ``-1`` if the specified user isn't on the channel


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set idle [getchanidle Foobar "#lamest"]

----

.. _tcl-getchanmode:

^^^^^^^^^^^^^^^^^^^^^
getchanmode <channel>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns string of the type ``+ntik key`` for the channel specified.

  Returns
     string of the type ``+ntik key`` for the channel specified


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set modes [getchanmode "#lamest"]

----

.. _tcl-jump:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
jump [server [[+]port [password]]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Jumps to the server specified, or (if none is specified) the next server in the bot's serverlist. If you prefix the port with a plus sign (e.g. +6697), SSL connection will be attempted.

  Returns
     nothing


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        jump irc.example.net +6697

----

.. _tcl-pushmode:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
pushmode <channel> <mode> [arg]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends out a channel mode change (ex: pushmode #lamest +o Foobar) through the bot's queuing system. All the mode changes will be sent out at once (combined into one line as much as possible) after the script finishes, or when ``flushmode`` is called.

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        pushmode "#lamest" +o Foobar

----

.. _tcl-flushmode:

^^^^^^^^^^^^^^^^^^^
flushmode <channel>
^^^^^^^^^^^^^^^^^^^

  Description
     Forces all previously pushed channel mode changes to be sent to the server, instead of when the script is finished (just for the channel specified).

  Returns
     nothing


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        flushmode "#lamest"

----

.. _tcl-topic:

^^^^^^^^^^^^^^^
topic <channel>
^^^^^^^^^^^^^^^

  Description
     Returns string containing the current topic of the specified channel.

  Returns
     string containing the current topic of the specified channel


  Module
     ``irc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set current_topic [topic "#lamest"]

----

.. _tcl-validchan:

^^^^^^^^^^^^^^^^^^^
validchan <channel>
^^^^^^^^^^^^^^^^^^^

  Description
     Checks if the bot has a channel record for the specified channel. Note that this does not necessarily mean that the bot is ON the channel.

  Returns
     ``1`` if the channel exists, ``0`` if not


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set known [validchan "#lamest"]

----

.. _tcl-isdynamic:

^^^^^^^^^^^^^^^^^^^
isdynamic <channel>
^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the channel is a dynamic channel; 0 otherwise.

  Returns
     ``1`` if the channel is a dynamic channel; ``0`` otherwise


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set dynamic [isdynamic "#lamest"]

----

.. _tcl-setudef:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setudef <flag/int/str> <name>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Initializes a user defined channel flag, string or integer setting. You can use it like any other flag/setting. IMPORTANT: Don't forget to reinitialize your flags/settings after a restart, or it'll be lost.

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setudef flag example-setting

----

.. _tcl-renudef:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
renudef <flag/int/str> <oldname> <newname>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Renames a user defined channel flag, string, or integer setting.

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        renudef flag example-setting renamed-setting

----

.. _tcl-deludef:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
deludef <flag/int/str> <name>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Deletes a user defined channel flag, string, or integer setting.

  Returns
     nothing


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        deludef flag example-setting

----

.. _tcl-getudefs:

^^^^^^^^^^^^^^^^^^^^^^^
getudefs [flag/int/str]
^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a list of user defined channel settings of the given type, or all of them if no type is given.

  Returns
     a list of user defined channel settings of the given type, or all of them if no type is given.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set flags [getudefs flag]

----

.. _tcl-chansettype:

^^^^^^^^^^^^^^^^^^^^^
chansettype <setting>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns the type of the setting you specify. The possible types are flag, int, str, pair. A flag type references a channel flag setting that can be set to either + or -. An int type is a channel  setting that is set to a number, such as ban-time. A str type is a  channel setting that stores a string, such as need-op. A pair type is a setting that holds a value couple, such as the flood settings.

  Returns
     The type of the setting you specify. The possible types are ``flag``, ``int``, ``str``, ``pair``. A flag type references a channel flag setting that can be set to either + or -. An int type is a channel  setting that is set to a number, such as ban-time. A str type is a  channel setting that stores a string, such as need-op. A pair type is a setting that holds a value couple, such as the flood settings.


  Module
     ``channels``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set type [chansettype autoop]

----

.. _tcl-isupport-get:

^^^^^^^^^^^^^^^^^^^^^^^^^
isupport get [key]
^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     - isupport get: Returns a flat key/value list (dict) of settings.
     - isupport get <key>: Returns the setting's value as a string. Throws an error if the key is not set.

  Returns
     string or dict, see description above


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set chantypes [isupport get CHANTYPES]

----

.. _tcl-isupport-isset:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
isupport isset <key>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns ``0``/1 depending on whether the key has a value.

  Returns
     ``0`` or ``1``


  Module
     ``server``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set supported [isupport isset CHANTYPES]

DCC Commands
------------

.. _putdcc:

.. _tcl-putdcc:

^^^^^^^^^^^^^^^^^^^^^^^^^^
putdcc <idx> <text> [-raw]
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends text to the idx specified. If -raw is specified, the text will be sent as is, without forced new lines or limits to line length.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putdcc 0 "Hello, foobar."

----

.. _tcl-putidx:

^^^^^^^^^^^^^^^^^^^^^^^^^^
putidx <idx> <text> -[raw]
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Alias for the putdcc_ command.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putidx 0 "Hello, foobar."

----

.. _tcl-dccbroadcast:

^^^^^^^^^^^^^^^^^^^^^^
dccbroadcast <message>
^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends a message to everyone on the party line across the botnet, in the form of ``\*\*\* <message>`` for local users, ``\*\*\* (Bot) <message>`` for users on other bots with version below 1.8.4, and ``(Bot) <message>`` for users on other bots with version 1.8.4+ and console log mode ``l`` enabled.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        dccbroadcast "Example botnet announcement"

----

.. _tcl-dccputchan:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dccputchan <channel> <message>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends your message to everyone on a certain channel on the botnet, in a form exactly like dccbroadcast does. Valid channels are 0 through 99999.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        dccputchan 0 "Hello from LamestBot"

----

.. _tcl-boot:

^^^^^^^^^^^^^^^^^^^^^^^^
boot <user@bot> [reason]
^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Boots a user from the partyline.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        boot foobar@LamestBot "Requested disconnect"

----

.. _tcl-dccsimul:

^^^^^^^^^^^^^^^^^^^^^
dccsimul <idx> <text>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Simulates text typed in by the dcc user specified. Note that in v0.9, this only simulated commands; now a command must be preceded by a ``.`` to be simulated.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        dccsimul 0 ".who"

----

.. _tcl-hand2idx:

^^^^^^^^^^^^^^^^^
hand2idx <handle>
^^^^^^^^^^^^^^^^^

  Description
     Returns the idx (a number greater than or equal to zero) for the user given if the user is on the party line in chat mode (even if she is currently on a channel or in chat off), the file area, or in the control of a script. -1 is returned if no idx is found. If the user is on multiple times, the oldest idx is returned.

  Returns
     the idx (a number greater than or equal to zero) for the user given if the user is on the party line in chat mode (even if she is currently on a channel or in chat off), the file area, or in the control of a script. ``-1`` is returned if no idx is found. If the user is on multiple times, the oldest idx is returned.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set idx [hand2idx foobar]

----

.. _tcl-idx2hand:

^^^^^^^^^^^^^^
idx2hand <idx>
^^^^^^^^^^^^^^

  Description
     Returns handle of the user with the given idx.

  Returns
     handle of the user with the given idx


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set handle [idx2hand 0]

----

.. _tcl-valididx:

^^^^^^^^^^^^^^
valididx <idx>
^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the idx currently exists; 0 otherwise.

  Returns
     ``1`` if the idx currently exists; ``0`` otherwise


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set valid [valididx 0]

----

.. _tcl-getchan:

^^^^^^^^^^^^^
getchan <idx>
^^^^^^^^^^^^^

  Description
     Returns the current party line channel for a user on the party line; ``0`` indicates he's on the group party line, ``-1`` means he has chat off, and a value from 1 to 99999 is a private channel.

  Returns
     the current party line channel for a user on the party line; ``0`` indicates he's on the group party line, ``-1`` means he has chat off, and a value from ``1`` to ``99999`` is a private channel


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set partyline_channel [getchan 0]

----

.. _tcl-setchan:

^^^^^^^^^^^^^^^^^^^^^^^
setchan <idx> <channel>
^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sets a party line user's channel. The party line user is not notified that she is now on a new channel. A channel name can be used (provided it exists).

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setchan 0 0

----

.. _tcl-console:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
console <idx> [channel] [console-modes]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Changes a dcc user's console mode, either to an absolute mode (like ``mpj``) or just adding/removing flags (like ``+pj`` or ``-moc`` or ``+mp-c``). The user's console channel view can be changed also (as long as the new channel is a valid channel).

  Returns
     a list containing the user's (new) channel view and (new) console modes, or nothing if that user isn't currently on the partyline


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set settings [console 0 "#lamest" +mp]

----

.. _tcl-resetconsole:

^^^^^^^^^^^^^^^^^^
resetconsole <idx>
^^^^^^^^^^^^^^^^^^

  Description
     Changes a dcc user's console mode to the default setting in the configfile.

  Returns
     a list containing the user's channel view and (new) console modes, or nothing if that user isn't currently on the partyline


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set settings [resetconsole 0]

----

.. _tcl-echo:

^^^^^^^^^^^^^^^^^^^
echo <idx> [status]
^^^^^^^^^^^^^^^^^^^

  Description
     Turns a user's echo on or off; the status has to be a 1 or 0.

  Returns
     new value of echo for that user (or the current value, if status was omitted)


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set echo_enabled [echo 0]

----

.. _tcl-strip:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
strip <idx> [+/-strip-flags]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Modifies the strip-flags for a user. The supported strip-flags are:

     .. list-table::
        :widths: 9 91
        :header-rows: 1

        * - ``strip-flags``
          - Description
        * - ``c``
          - remove all color codes
        * - ``b``
          - remove all boldface codes
        * - ``r``
          - remove all reverse video codes
        * - ``u``
          - remove all underline codes
        * - ``a``
          - remove all ANSI codes
        * - ``g``
          - remove all ctrl-g (bell) codes
        * - ``o``
          - remove all ordinary codes (ctrl+o, terminates bold/color/..)
        * - ``i``
          - remove all italics codes
        * - ``\*``
          - remove all of the above

  Returns
     new strip-flags for the specified user (or the current flags, if strip-flags was omitted)


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set strip_flags [strip 0 +c]

----

.. _tcl-page:

^^^^^^^^^^^^^^^^^^^
page <idx> [number]
^^^^^^^^^^^^^^^^^^^

  Description
     This allows you to slow down the number of lines the bot sends.

     to a user at once via the partyline. When enabled, any commands that send

     greater than the specified number of lines will stop when that number is

     reached and wait for the user to type another command (or press enter) to

     continue. If the user has too many pending lines, he may be booted off the

     bot.

  Returns
     new value of page lines for that user (or the current value, if

     status was omitted)


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set page_lines [page 0 20]

----

.. _tcl-putbot:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
putbot <bot-nick> <message>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sends a message across the botnet to another bot. If no script intercepts the message on the other end, the message is ignored.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putbot OtherBot "Hello from LamestBot"

----

.. _tcl-putallbots:

^^^^^^^^^^^^^^^^^^^^
putallbots <message>
^^^^^^^^^^^^^^^^^^^^

  Description
     Sends a message across the botnet to all bots. If no script intercepts the message on the other end, the message is ignored.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        putallbots "example-message Hello"

----

.. _tcl-killdcc:

^^^^^^^^^^^^^
killdcc <idx>
^^^^^^^^^^^^^

  Description
     Kills a partyline or file area connection.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        killdcc 0

----

.. _tcl-bots:

^^^^
bots
^^^^

  Description
     Returns list of the bots currently connected to the botnet.

  Returns
     list of the bots currently connected to the botnet


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set linked_bots [bots]

----

.. _tcl-botlist:

^^^^^^^
botlist
^^^^^^^

  Description
     Returns a list of bots currently on the botnet. Each item in the list is a sublist with four elements: bot, uplink, version, and sharing status:

  Returns
     a list of bots currently on the botnet. Each item in the list is a sublist with four elements: ``bot``, ``uplink``, ``version``, and ``sharing status``:

     .. list-table::
        :widths: 18 82
        :header-rows: 1

        * - Element
          - Description
        * - ``bot``
          - the bot's botnetnick
        * - ``uplink``
          - the bot the bot is connected to
        * - ``version``
          - it's current numeric version
        * - ``sharing``
          - a ``+`` if the bot is a sharebot; ``-`` otherwise


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set botnet [botlist]

----

.. _tcl-islinked:

^^^^^^^^^^^^^^
islinked <bot>
^^^^^^^^^^^^^^

  Description
     Returns ``1`` if the bot is currently linked; 0 otherwise.

  Returns
     ``1`` if the bot is currently linked; ``0`` otherwise


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set linked [islinked OtherBot]

----

.. _tcl-dccused:

^^^^^^^
dccused
^^^^^^^

  Description
     Returns number of dcc connections currently in use.

  Returns
     number of dcc connections currently in use


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set connections [dccused]

----

.. _tcl-dcclist:

^^^^^^^^^^^^^^
dcclist [type]
^^^^^^^^^^^^^^

  Description
     Returns a list of active connections, each item in the list is a sublist containing seven elements:

  Returns
     a list of active connections, each item in the list is a sublist containing seven elements:
     ``{<idx> <handle> <hostname> <[+]port> <type> {<other>} <timestamp>}``.

     The types are: ``chat``, ``bot``, ``files``, ``file_receiving``, ``file_sending``, ``file_send_pending``, ``script``, ``socket`` (these are connections that have not yet been put under 'control'), ``telnet``, and ``server``. The timestamp is in unixtime format.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set connections [dcclist chat]

----

.. _tcl-socklist:

^^^^^^^^^^^^^^^
socklist [type]
^^^^^^^^^^^^^^^

  Description
     Returns a list of active connections, each item in the list is a sublist containing eight elements (in dict-readable format). The order of items returned should not be considered static or permanent, so it is recommended to access the items as key/value pairs with the dict command, as opposed to something like lindex, to extract values. The possible keys returned are:

  Returns
     a list of active connections, each item in the list is a sublist containing eight elements (in dict-readable format). The order of items returned should not be considered static or permanent, so it is recommended to access the items as key/value pairs with the dict command, as opposed to something like lindex, to extract values. The possible keys returned are:

     .. list-table::
        :widths: 15 85
        :header-rows: 1

        * - Key
          - Description
        * - ``idx``
          - integer value assigned to Eggdrop connections
        * - ``handle``
          - possible values are (telnet), (bots), (users),
            (script) for a listening socket, or the handle of the
            connected user for an established connection
        * - ``host``
          - the hostname of the connection, if it is known;
            otherwise a *
        * - ``ip``
          - the ip of the connection
        * - ``port``
          - the port number associated with the connection (local
            port for listening connections, remote port for server
            connections.
        * - ``secure``
          - 1 if SSL/TLS is used for the connect; 0 otherwise
        * - ``type``
          - the type of connection (TELNET, CHAT, SERVER, etc)
        * - ``info``
          - extra information associated with the connection
        * - ``time``
          - timestamp of when the socket was established

  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set sockets [socklist]


 Module: core

----

.. _tcl-whom:

^^^^^^^^^^^
whom <chan>
^^^^^^^^^^^

  Description
     Returns list of people on the botnet who are on that channel. 0 is the default party line. Each item in the list is a sublist with six elements: nickname, bot, hostname, access flag (``-``, ``@``, ``+``, or ``*``), minutes idle, and away message (blank if the user is not away). If you specify * for channel, every user on the botnet is returned with an extra argument indicating the channel the user is on.

  Returns
     list of people on the botnet who are on that channel. ``0`` is the default party line. Each item in the list is a sublist with six elements: ``nickname``, ``bot``, ``hostname``, ``access flag`` ('-', '@', '+', or '*'), minutes idle, and away message (blank if the user is not away). If you specify ``*`` for channel, every user on the botnet is returned with an extra argument indicating the channel the user is on.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set users [whom 0]

----

.. _tcl-getdccidle:

^^^^^^^^^^^^^^^^
getdccidle <idx>
^^^^^^^^^^^^^^^^

  Description
     Returns number of seconds the dcc chat/file system/script user has been idle.

  Returns
     number of seconds the dcc chat/file system/script user has been idle


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set idle_seconds [getdccidle 0]

----

.. _tcl-getdccaway:

^^^^^^^^^^^^^^^^
getdccaway <idx>
^^^^^^^^^^^^^^^^

  Description
     Returns away message for a dcc chat user (or ``""`` if the user is not set away).

  Returns
     away message for a dcc chat user (or ``""`` if the user is not set away)


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set away_message [getdccaway 0]

----

.. _tcl-setdccaway:

^^^^^^^^^^^^^^^^^^^^^^^^^^
setdccaway <idx> <message>
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sets a party line user's away message and marks them away. If set to ``""``, the user is marked as no longer away.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setdccaway 0 "Away for a moment"

----

.. _tcl-connect:

^^^^^^^^^^^^^^^^^^^^^^^^
connect <host> <[+]port>
^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Makes an outgoing connection attempt and creates a dcc entry for it. A ``control`` command should be used immediately after a successful ``connect`` so no input is lost. If the port is prefixed with a plus sign, SSL encrypted connection will be attempted.

  Returns
     idx of the new connection


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set idx [connect 127.0.0.1 9000]

----

.. _tcl-listen:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
listen [ip] <port> <type> [options [flag]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Opens a listening port to accept incoming telnets; type must be one of ``bots``, ``all``, ``users``, ``script``, or ``off``. Prefixing the port with a plus sign will make eggdrop accept SSL connections on it. An IP may optionally be listed before the mandatory port argument. If no IP is specified, all available interfaces are used.

       ``listen [ip] <port> bots [mask]``

         **Description:** accepts connections from bots only; the optional mask is used to identify permitted bot names. If the mask begins with '@', it is interpreted to be a mask of permitted hosts to accept connections from.

         **Returns:** port number or error message

       ``listen [ip] <port> users [mask]``
       
         **Description:** accepts connections from users only (no bots); the optional mask is used to identify permitted nicknames. If the mask begins with '@', it is interpreted to be a mask of permitted hosts to accept connections from.

         **Returns:** port number or error message

       ``listen [ip] <port> all [mask]``

         **Description:** accepts connections from anyone; the optional mask is used to identify permitted nicknames/botnames. If the mask begins with '@', it is interpreted to be a mask of permitted hosts to accept connections from.

         **Returns:** port number or error message

       ``listen [ip] <port> script <proc> [flag]``

         **Description:** accepts connections which are immediately routed to a proc. The proc is called with one parameter: the idx of the new connection. The optional flag parameter currently only accepts 'pub' as a value. By specifying 'pub' as a flag, Eggdrop will skip the ident check for the user regardless of settings in the config file. This will allow any user to attempt a connection, and result in Eggdrop using "-telnet!telnet@host" instead of "-telnet!<ident>@host" as a hostmask to match against the user.

         **Returns:** port number or error message

       ``listen [ip] <port> off``

         **Description:** stop listening on a port

       **Returns:** nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set port [listen 127.0.0.1 3333 users]

----

.. _tcl-dccdumpfile:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dccdumpfile <idx> <filename>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Dumps out a file from the text directory to a dcc chat user. The flag matching that's used everywhere else works here, too.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        dccdumpfile 0 help.txt

Notes Module
------------

.. _tcl-notes:

^^^^^^^^^^^^^^^^^^^^^^^^^
notes <user> [numberlist]
^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns -1 if no such user, -2 if notefile failure. If a numberlist is not specified, the number of notes stored for the user is returned. Otherwise, a list of sublists containing information about notes stored for the user is returned. Each sublist is in the format of:

  Returns
     ``-1`` if no such user, ``-2`` if notefile failure. If a numberlist is not specified, the number of notes stored for the user is returned. Otherwise, a list of sublists containing information about notes stored for the user is returned. Each sublist is in the format of::

           ``{<from> <timestamp> <note text>}``


  Module
     ``notes``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set note_count [notes foobar]

----

.. _tcl-erasenotes:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
erasenotes <user> <numberlist>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Erases some or all stored notes for a user. Use ``-`` to erase all notes.

  Returns
     ``-1`` if no such user, ``-2`` if notefile failure, ``0`` if no such note, or number of erased notes.


  Module
     ``notes``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set erased [erasenotes foobar "1-3"]

----

.. _tcl-listnotes:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
listnotes <user> <numberlist>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Lists existing notes according to the numberlist (ex: ``2-4;8;16-``).

  Returns
     ``-1`` if no such user, ``-2`` if notefile failure, ``0`` if no such note, list of existing notes.


  Module
     ``notes``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set notes [listnotes foobar "1-3"]

----

.. _tcl-storenote:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
storenote <from> <to> <msg> <idx>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Stores a note for later reading, notifies idx of any results (use idx -1 for no notify).

  Returns
     ``0`` on success; non-``0`` on failure


  Module
     ``notes``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set status [storenote foobar foobar "Remember the meeting." -1]

Assoc Module
------------

.. _tcl-assoc:

^^^^^^^^^^^^^^^^^^^
assoc <chan> [name]
^^^^^^^^^^^^^^^^^^^

  Description
     Sets the name associated with a botnet channel, if you specify one.

  Returns
     current name for that channel, if any


  Module
     ``assoc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set name [assoc 1 lamest]

----

.. _tcl-killassoc:

^^^^^^^^^^^^^^^^
killassoc <chan>
^^^^^^^^^^^^^^^^

  Description
     Removes the name associated with a botnet channel, if any exists. Use ``killassoc &`` to kill all assocs.

  Returns
     nothing


  Module
     ``assoc``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        killassoc 1

Compress Module
---------------

.. _tcl-compressfile:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
compressfile [-level <level>] <src-file> [target-file]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Compresses a file. The optional level specifies the compression mode from 0 (minimum CPU usage and compression) through 9 (maximum CPU usage and compression). If target-file is omitted, src-file is overwritten.

  Returns
     nothing

  See also
     :ref:`uncompressfile <tcl-uncompressfile>`, :ref:`iscompressed <tcl-iscompressed>`


  Module
     ``compress``


  .. admonition:: Example — Compress a file
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        compressfile -level 6 example.txt example.txt.gz

----

.. _tcl-uncompressfile:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
uncompressfile <src-file> [target-file]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Uncompresses a file. If target-file is omitted, src-file is overwritten.

  Returns
     nothing

  See also
     :ref:`compressfile <tcl-compressfile>`, :ref:`iscompressed <tcl-iscompressed>`


  Module
     ``compress``


  .. admonition:: Example — Uncompress a file
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        uncompressfile example.txt.gz example.txt

----

.. _tcl-iscompressed:

^^^^^^^^^^^^^^^^^^^^^^^
iscompressed <filename>
^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Determines whether ``<filename>`` is gzip compressed. .

  Returns
     ``1`` if it is, ``0`` if it isn't, and ``2`` if some kind of error prevented the checks from succeeding.


  Module
     ``compress``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set compressed [iscompressed example.gz]

Filesys Module
--------------

.. _tcl-setpwd:

^^^^^^^^^^^^^^^^^^
setpwd <idx> <dir>
^^^^^^^^^^^^^^^^^^

  Description
     Changes the directory of a file system user, in exactly the same way as a ``cd`` command would. The directory can be specified relative or absolute.

  Returns
     nothing


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setpwd 0 /

----

.. _tcl-getpwd:

^^^^^^^^^^^^
getpwd <idx>
^^^^^^^^^^^^

  Description
     Returns the current directory of a file system user.

  Returns
     the current directory of a file system user


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set directory [getpwd 0]

----

.. _tcl-getfiles:

^^^^^^^^^^^^^^
getfiles <dir>
^^^^^^^^^^^^^^

  Description
     Returns a list of files in the directory given; the directory is relative to dcc-path.
  Returns
     a list of files in the directory given; the directory is relative to dcc-path


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set files [getfiles /]

----

.. _tcl-getdirs:

^^^^^^^^^^^^^
getdirs <dir>
^^^^^^^^^^^^^

  Description
     Returns a list of subdirectories in the directory given; the directory is relative to dcc-path.

  Returns
     a list of subdirectories in the directory given; the directory is relative to dcc-path


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set directories [getdirs /]

----

.. _tcl-dccsend:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dccsend <filename> <ircnick>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Attempts to start a dcc file transfer to the given nick; the filename must be specified either by full pathname or in relation to the bot's startup directory.

  Returns

     .. list-table::
        :widths: 9 91
        :header-rows: 1

        * - Return value
          - Description
        * - ``0``
          - success
        * - ``1``
          - the dcc table is full (too many connections)
        * - ``2``
          - can't open a socket for the transfer
        * - ``3``
          - the file doesn't exist
        * - ``4``
          - the file was queued for later transfer, which means that person has
            too many file transfers going right now
        * - ``5``
          - the file could not be opened or temporary file could not be created


  Module
     ``transfer``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [dccsend example.txt Foobar]

----

.. _tcl-filesend:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
filesend <idx> <filename> [ircnick]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Like dccsend, except it operates for a current filesystem user, and the filename is assumed to be a relative path from that user's current directory.

  Returns
     ``0`` on failure; ``1`` on success (either an immediate send or a queued send)


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [filesend 0 example.txt Foobar]

----

.. _tcl-fileresend:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
fileresend <idx> <filename> [ircnick]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Functions like filesend, only that it sends a DCC RESEND instead of a DCC SEND, which allows people to resume aborted file transfers if their client supports that protocol. ircII/BitchX/etc. support it; mIRC does not.

  Returns
     ``0`` on failure; ``1`` on success (either an immediate send or a queued send)


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [fileresend 0 example.txt Foobar]

----

.. _tcl-setdesc:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
setdesc <dir> <file> <desc>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sets the description for a file in a file system directory; the directory is relative to dcc-path.

  Returns
     nothing


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setdesc / example.txt "Example file"

----

.. _tcl-getdesc:

^^^^^^^^^^^^^^^^^^^^
getdesc <dir> <file>
^^^^^^^^^^^^^^^^^^^^

  Description
     Returns the description for a file in the file system, if one exists.

  Returns
     the description for a file in the file system, if one exists


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set description [getdesc / example.txt]

----

.. _tcl-setowner:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setowner <dir> <file> <handle>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Changes the owner for a file in the file system; the directory is relative to dcc-path.

  Returns
     nothing


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setowner / example.txt foobar

----

.. _tcl-getowner:

^^^^^^^^^^^^^^^^^^^^^
getowner <dir> <file>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns the owner of a file in the file system.

  Returns
     the owner of a file in the file system


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set owner [getowner / example.txt]

----

.. _tcl-setlink:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
setlink <dir> <file> <link>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Creates or changes a linked file (a file that actually exists on another bot); the directory is relative to dcc-path.

  Returns
     nothing


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setlink / example.txt OtherBot:/example.txt

----

.. _tcl-getlink:

^^^^^^^^^^^^^^^^^^^^
getlink <dir> <file>
^^^^^^^^^^^^^^^^^^^^

  Description
     Returns the link for a linked file, if it exists.

  Returns
     the link for a linked file, if it exists


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set link [getlink / example.txt]

----

.. _tcl-getfileq:

^^^^^^^^^^^^^^^^^
getfileq <handle>
^^^^^^^^^^^^^^^^^

  Description
     Returns list of files queued by someone; each item in the list will be a sublist with two elements: nickname the file is being sent to and the filename.

  Returns
     list of files queued by someone; each item in the list will be a sublist with two elements: nickname the file is being sent to and the filename


  Module
     ``transfer``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set queued_files [getfileq foobar]

----

.. _tcl-getfilesendtime:

^^^^^^^^^^^^^^^^^^^^^
getfilesendtime <idx>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns the unixtime value from when a file transfer started, or a negative number:

  Returns
     the unixtime value from when a file transfer started, or a negative number:

     .. list-table::
        :widths: 8 92
        :header-rows: 1

        * - Return value
          - Description
        * - ``-1``
          - no matching transfer with the specified idx was found
        * - ``-2``
          - the idx matches an entry which is not a file transfer


  Module
     ``transfer``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [getfilesendtime 0]

----

.. _tcl-mkdir:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
mkdir <directory> [<required-flags> [channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Creates a directory in the file system. Only users with the required flags may access it.

  Returns

     .. list-table::
        :widths: 8 92
        :header-rows: 1

        * - Return value
          - Description
        * - ``0``
          - success
        * - ``1``
          - can't create directory
        * - ``2``
          - directory exists but is not a directory
        * - ``-3``
          - could not open filedb


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [mkdir /examples]

----

.. _tcl-rmdir:

^^^^^^^^^^^^^^^^^
rmdir <directory>
^^^^^^^^^^^^^^^^^

  Description
     Removes a directory from the file system.

  Returns
     ``0`` on success; ``1`` on failure


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [rmdir /examples]

----

.. _tcl-mv:

^^^^^^^^^^^^^^^^^^^^^^^
mv <file> <destination>
^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Moves a file from its source to the given destination. The file can also be a mask, such as ``/incoming``/\*, provided the destination is a directory.

  Returns
     If the command was successful, the number of files moved will be returned. Otherwise, a negative number will be returned:

     .. list-table::
        :widths: 8 92
        :header-rows: 1

        * - Return value
          - Description
        * - ``-1``
          - invalid source file
        * - ``-2``
          - invalid destination
        * - ``-3``
          - destination file exists
        * - ``-4``
          - no matches found


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set moved [mv /incoming/example.txt /files/example.txt]

----

.. _tcl-cp:

^^^^^^^^^^^^^^^^^^^^^^^
cp <file> <destination>
^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Copies a file from its source to the given destination. The file can also be a mask, such as ``/incoming``/\*, provided the destination is a directory.

  Returns
     If the command was successful, the number of files copied will be returned. Otherwise, a negative number will be returned:

     .. list-table::
        :widths: 8 92
        :header-rows: 1

        * - Return value
          - Description
        * - ``-1``
          - invalid source file
        * - ``-2``
          - invalid destination
        * - ``-3``
          - destination file exists
        * - ``-4``
          - no matches found


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set copied [cp /incoming/example.txt /files/example.txt]

----

.. _tcl-getflags:

^^^^^^^^^^^^^^
getflags <dir>
^^^^^^^^^^^^^^

  Description
     Returns the flags required to access a directory.

  Returns
     the flags required to access a directory


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set required_flags [getflags /]

----

.. _tcl-setflags:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
setflags <dir> [<flags> [channel]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Sets the flags required to access a directory.

  Returns
     ``0`` on success; ``-1`` or ``-3`` on failure


  Module
     ``filesys``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        setflags /examples +o "#lamest"

PBKDF2 Module
-------------

.. _tcl-encpass2:

^^^^^^^^^^^^^^^
encpass2 <pass>
^^^^^^^^^^^^^^^

  Description
     Returns a hash in the format of ``$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>`` where digest is the digest set in the config variable pbkdf2-method, rounds is the number of rounds set in the config variable pbkdf2-rounds, salt is the base64 salt used to generate the hash, and hash is the generated base64 hash.


  Returns
     a hash in the format of ``$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>`` where digest is the digest set in the config variable pbkdf2-method, rounds is the number of rounds set in the config variable pbkdf2-rounds, salt is the base64 salt used to generate the hash, and hash is the generated base64 hash.


  Module
     ``pbkdf2``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set hash [encpass2 "example-password"]

----

.. _tcl-pbkdf2:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
pbkdf2 [-bin] <pass> <salt> <rounds> <digest>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a derived key from the provided ``pass`` string using ``salt`` and ``rounds`` count as specified in RFC 2898 as a hexadecimal string. Using the optional -bin flag will return the result as binary data.

  Returns
     a derived key from the provided ``pass`` string using ``salt`` and ``rounds`` count as specified in RFC 2898 as a hexadecimal string. Using the optional -bin flag will return the result as binary data.


  Module
     ``pbkdf2``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set key [pbkdf2 "example-password" "example-salt" 10000 sha256]

Miscellaneous Commands
----------------------

.. _tcl-bind:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
bind <type> <flags> <keyword/mask> [proc-name]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     You can use the ``bind`` command to attach Tcl procedures to certain events. flags are the flags the user must have to trigger the event (if applicable). proc-name is the name of the Tcl procedure to call for this command (see below for the format of the procedure call). If the proc-name is omitted, no binding is added. Instead, the current binding is returned (if it's stackable, a list of the current bindings is returned).

  Returns
     name of the command that was added, or (if proc-name was omitted), a list of the current bindings for this command


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind pub - !hello example_hello

----

.. _tcl-unbind:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
unbind <type> <flags> <keyword/mask> <proc-name>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes a previously created bind.

  Returns
     name of the command that was removed


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        unbind pub - !hello example_hello

----

.. _tcl-binds:

^^^^^^^^^^^^^^^^^
binds [type/mask]
^^^^^^^^^^^^^^^^^

  Description
     By default, lists Tcl binds registered with the Eggdrop. You can specify ``all`` to view all binds, ``tcl`` to view Tcl binds, and ``python`` to view Python binds. Alternately, you can specify a bind type (pub, msg, etc) to view all binds that match that type of bind, or a mask that is matched against the command associated with the bind.

  Returns
     a list of Tcl binds, each item in the list is a sublist of five elements:
           ``{<type> <flags> <name> <hits> <proc>}``


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set public_binds [binds pub]

----

.. _tcl-logfile:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
logfile [<modes> <channel> <filename>]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Creates a new logfile, which will log the modes given for the channel listed. If no logfile is specified, a list of existing logfiles will be returned. ``*`` indicates all channels. You can also change the modes and channel of an existing logfile with this command. Entering a blank mode and channel (``""``) makes the bot stop logging there.

     Logfile flags:

     .. list-table::
        :widths: 7 93
        :header-rows: 1

        * - ``modes``
          - Description
        * - ``b``
          - information about bot linking and userfile sharing
        * - ``c``
          - commands
        * - ``d``
          - misc debug information
        * - ``g``
          - raw outgoing share traffic
        * - ``h``
          - raw incoming share traffic
        * - ``j``
          - joins, parts, quits, topic changes, and netsplits on the channel
        * - ``k``
          - kicks, bans, and mode changes on the channel
        * - ``l``
          - linked bot messages
        * - ``m``
          - private msgs, notices and ctcps to the bot
        * - ``o``
          - misc info, errors, etc (IMPORTANT STUFF)
        * - ``p``
          - public text on the channel
        * - ``r``
          - raw incoming server traffic
        * - ``s``
          - server connects, disconnects, and notices
        * - ``t``
          - raw incoming botnet traffic
        * - ``u``
          - raw outgoing botnet traffic
        * - ``v``
          - raw outgoing server traffic
        * - ``w``
          - wallops (make sure the bot sets +w in init-server)
        * - ``x``
          - file transfers and file-area commands

  Returns
     filename of logfile created, or, if no logfile is specified, a list of logfiles such as: ``{mco * eggdrop.log}`` ``{jp #lamest lame.log}``


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set logfiles [logfile]

----

.. _tcl-maskhost:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
maskhost <nick!user@host> [masktype]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns masked hostmask for the string given according to the masktype (the default is 3).

  Returns
     masked hostmask for the string given according to the masktype (the default is 3).

     Available types are:

     .. list-table::
        :widths: 8 92
        :header-rows: 1

        * - ``masktype``
          - Description
        * - ``0``
          - \*!user\@host
        * - ``1``
          - \*!*user\@host
        * - ``2``
          - \*!*\@host
        * - ``3``
          - \*!*user\@*.host
        * - ``4``
          - \*!*\@*.host
        * - ``5``
          - nick!user\@host
        * - ``6``
          - nick!*user\@host
        * - ``7``
          - nick!*\@host
        * - ``8``
          - nick!*user\@*.host
        * - ``9``
          - nick!*\@*.host

     You can also specify types from 10 to 19 which correspond to types
     0 to 9, but instead of using a * wildcard to replace portions of the
     host, only numbers in hostnames are replaced with the '?' wildcard.
     Same is valid for types 20-29, but instead of '?', the '\*' wildcard
     will be used. Types 30-39 set the host to '\*'.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set mask [maskhost "Foobar!foobar@127.0.0.1"]

----

.. _tcl-timer:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
timer <minutes> <tcl-command> [count [timerName]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Executes the given Tcl command after a certain number of minutes have passed, at the top of the minute (ie, if a timer is started at 10:03:34 with 1 minute specified, it will execute at 10:04:00. If a timer is started at 10:06:34 with 2 minutes specified, it will execute at 10:08:00). If count is specified, the command will be executed count times with the given interval in between. If you specify a count of 0, the timer will repeat until it's removed with killtimer or until the bot is restarted. If timerName is specified, it will become the unique identifier for the timer. If no timerName is specified, Eggdrop will assign a timerName in the format of ``timer<integer>``.

  Returns
     a ``timerName``


  Module
     ``core``


  .. admonition:: Example — Schedule a timer
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set id [timer 5 {putlog "Example timer fired."}]

----

.. _tcl-utimer:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
utimer <seconds> <tcl-command> [count [timerName]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Executes the given Tcl command after a certain number of seconds have passed. If count is specified, the command will be executed count times with the given interval in between. If you specify a count of 0, the utimer will repeat until it's removed with killutimer or until the bot is restarted. If timerName is specified, it will become the unique identifier for the timer. If timerName is not specified, Eggdrop will assign a timerName in the format of ``timer<integer>``.

  Returns
     a ``timerName``


  Module
     ``core``


  .. admonition:: Example — Schedule a short timer
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set id [utimer 30 {putlog "Example timer fired."}]

----

.. _tcl-timers:

^^^^^^
timers
^^^^^^

  Description
     Lists all active minutely timers.

  Returns
     a list of active minutely timers, with each timer sub-list containing the number of minutes left until activation, the command that will be executed, the timerName, and the remaining number of repeats.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set active_timers [timers]

----

.. _tcl-utimers:

^^^^^^^
utimers
^^^^^^^

  Description
     Lists all active secondly timers.

  Returns
     a list of active secondly timers, with each timer sub-list containing the number of seconds left until activation, the command that will be executed, the timerName, and the remaining number of repeats.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set active_timers [utimers]

----

.. _tcl-killtimer:

^^^^^^^^^^^^^^^^^^^^^
killtimer <timerName>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes the timerName minutely timer from the timer list.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set timer_id [timer 5 {putlog "Example timer fired."}]
        killtimer $timer_id

----

.. _tcl-killutimer:

^^^^^^^^^^^^^^^^^^^^^^
killutimer <timerName>
^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes the timerName secondly timer from the timer list.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set timer_id [utimer 30 {putlog "Example timer fired."}]
        killutimer $timer_id

----

.. _tcl-unixtime:

^^^^^^^^
unixtime
^^^^^^^^

  Description
     Returns the current Unix timestamp.


  Returns
     a long integer which represents the number of seconds that have passed since 00:00 Jan 1, 1970 (GMT).


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set now [unixtime]

----

.. _tcl-duration:

^^^^^^^^^^^^^^^^^^
duration <seconds>
^^^^^^^^^^^^^^^^^^

  Description
     Returns the number of seconds converted into years, weeks, days, hours, minutes, and seconds. 804600 seconds is turned into 1 week 2 days 7 hours 30 minutes.

  Returns
     the number of seconds converted into years, weeks, days, hours, minutes, and seconds. 804600 seconds is turned into 1 week 2 days 7 hours 30 minutes.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set readable [duration 3600]

----

.. _tcl-strftime:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
strftime <formatstring> [time]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns a formatted string of time using standard strftime format. If time is specified, the value of the specified time is used. Otherwise, the current time is used. Note: The implementation of strftime varies from platform to platform, so the user should only use POSIX-compliant format specifiers to ensure fully portable code.

  Returns
     a formatted string of time using standard strftime format. If time is specified, the value of the specified time is used. Otherwise, the current time is used. Note: The implementation of strftime varies from platform to platform, so the user should only use POSIX-compliant format specifiers to ensure fully portable code.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set date [strftime "%Y-%m-%d"]

----

.. _tcl-ctime:

^^^^^^^^^^^^^^^^
ctime <unixtime>
^^^^^^^^^^^^^^^^

  Description
     Returns a formatted date/time string based on the current locale settings from the unixtime string given; for example ``Fri Aug 3 11:34:55 1973``.

  Returns
     a formatted date/time string based on the current locale settings from the unixtime string given; for example ``Fri Aug 3 11:34:55 1973``


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set date [ctime [unixtime]]

----

.. _tcl-myip:

^^^^
myip
^^^^

  Description
     Returns a long number representing the bot's IP address, as it might appear in (for example) a DCC request.

  Returns
     a long number representing the bot's IP address, as it might appear in (for example) a DCC request


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [myip]

----

.. _tcl-rand:

^^^^^^^^^^^^
rand <limit>
^^^^^^^^^^^^

  Description
     Returns a random integer between 0 and limit-1. Limit must be greater than 0 and equal to or less than RAND_MAX, which is generally 2147483647. The underlying pseudo-random number generator is not cryptographically secure.

  Returns
     a random integer between ``0`` and ``limit-1``. Limit must be greater than ``0`` and equal to or less than ``RAND_MAX``, which is generally 2147483647. The underlying pseudo-random number generator is not cryptographically secure.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set value [rand 100]

----

.. _tcl-control:

^^^^^^^^^^^^^^^^^^^^^^^
control <idx> <command>
^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Removes an idx from the party line and sends all future input to the Tcl command given. The command will be called with two parameters: the idx and the input text. The command should return ``0`` to indicate success and 1 to indicate that it relinquishes control of the user back to the bot. If the input text is blank (``""``), it indicates that the connection has been dropped. Also, if the input text is blank, never call killdcc on it, as it will fail with "invalid idx".

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        control 0 example_proc

----

.. _tcl-sendnote:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
sendnote <from> <to[@bot]> <message>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Simulates what happens when one user sends a note to another.

  Returns

     .. list-table::
        :widths: 8 92
        :header-rows: 1

        * - Return value
          - Description
        * - ``0``
          - the send failed
        * - ``1``
          - the note was delivered locally or sent to another bot
        * - ``2``
          - the note was stored locally
        * - ``3``
          - the user's notebox is too full to store a note
        * - ``4``
          - a Tcl binding caught the note
        * - ``5``
          - the note was stored because the user is away


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [sendnote foobar foobar "Example note"]

----

.. _tcl-link:

^^^^^^^^^^^^^^^^^^^^
link [via-bot] <bot>
^^^^^^^^^^^^^^^^^^^^

  Description
     Attempts to link to another bot directly. If you specify a via-bot, it tells the via-bot to attempt the link.

  Returns
     ``1`` if the link will be attempted; ``0`` otherwise


  Module
     ``core``


  .. admonition:: Example — Link a bot
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set started [link OtherBot]

----

.. _tcl-unlink:

^^^^^^^^^^^^^^^^^^^^^^
unlink <bot> [comment]
^^^^^^^^^^^^^^^^^^^^^^

  Description
     Attempts to unlink a bot from the botnet. If you specify a comment, it will appear with the unlink message on the botnet.

  Returns
     ``1`` on success; ``0`` otherwise


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set removed [unlink OtherBot "Maintenance"]

----

.. _tcl-encrypt:

^^^^^^^^^^^^^^^^^^^^^^
encrypt <key> <string>
^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns encrypted string (using the currently loaded encryption module), encoded into ASCII using base-64. As of v1.8.4, the default blowfish encryption module can use either the older ECB mode (currently used by default for compatibility reasons), or the more recent and more-secure CBC mode. You can explicitly request which encryption mode to use by prefixing the encryption key with either ``ecb:`` or ``cbc:``, or by using the blowfish-use-mode setting in the config file. Note: the default encryption mode for this function is planned to transition from ECB to CBC in v1.9.0.

  Returns
     encrypted string (using the currently loaded encryption module), encoded into ASCII using base-64. As of v1.8.4, the default blowfish encryption module can use either the older ECB mode (currently used by default for compatibility reasons), or the more recent and more-secure CBC mode. You can explicitly request which encryption mode to use by prefixing the encryption key with either ``ecb:`` or ``cbc:``, or by using the blowfish-use-mode setting in the config file. Note: the default encryption mode for this function is planned to transition from ECB to CBC in v1.9.0.


  Module
     ``encryption``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [encrypt "example-password" "Example text"]

----

.. _tcl-decrypt:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
decrypt <key> <encrypted-base64-string>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Returns decrypted string (using the currently loaded encryption module). If the default blowfish encryption module is used, this automatically picks the right decryption mode. You may still prefix the key with ``ecb:`` or ``cbc:`` or use the blowfish-use-mode setting in the config file (see the encrypt command for more detailed information).

  Returns
     decrypted string (using the currently loaded encryption module). If the default blowfish encryption module is used, this automatically picks the right decryption mode. You may still prefix the key with ``ecb:`` or ``cbc:`` or use the blowfish-use-mode setting in the config file (see the encrypt command for more detailed information).


  Module
     ``encryption``


  .. admonition:: Example — Encrypt and decrypt text
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set encrypted [encrypt "example-password" "Example text"]
        set plain [decrypt "example-password" $encrypted]

----

.. _tcl-encpass:

^^^^^^^^^^^^^^^^^^
encpass <password>
^^^^^^^^^^^^^^^^^^

  Description
     Returns encrypted string (using the currently loaded encryption module).

  Returns
     encrypted string (using the currently loaded encryption module)


  Module
     ``encryption``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set encrypted [encpass "example-password"]

----

.. _tcl-die:

^^^^^^^^^^^^
die [reason]
^^^^^^^^^^^^

  Description
     Causes the bot to log a fatal error and exit completely. If no reason is given, ``EXIT`` is used.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        die

----

.. _tcl-unames:

^^^^^^
unames
^^^^^^

  Description
     Returns the current operating system the bot is using.

  Returns
     the current operating system the bot is using


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [unames]

----

.. _tcl-dnslookup:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dnslookup <ip-address/hostname> <proc> [[arg1] [arg2] ... [argN]]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     This issues an asynchronous dns lookup request. The command will block if dns module is not loaded; otherwise it will either return immediately or immediately call the specified proc (e.g. if the lookup is already cached).

     As soon as the request completes, the specified proc will be called as follows:

       <proc> <ipaddress> <hostname> <status> [[arg1] [arg2] ... [argN]]

     status is ``1`` if the lookup was successful and 0 if it wasn't. All additional parameters (called arg1, arg2 and argN above) get appended to the proc's other parameters.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example — Resolve a hostname
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        proc example_dns_result {ip host status} {
          putlog "DNS result: $host -> $ip (status $status)"
        }
        dnslookup irc.example.net example_dns_result

----

.. _tcl-md5:

^^^^^^^^^^^^
md5 <string>
^^^^^^^^^^^^

  Description
     Returns the 128 bit MD5 message-digest of the specified string.

  Returns
     the 128 bit MD5 message-digest of the specified string


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set digest [md5 "example text"]

----

.. _tcl-callevent:

^^^^^^^^^^^^^^^^^
callevent <event>
^^^^^^^^^^^^^^^^^

  Description
     Triggers the evnt bind manually for a certain event. You can call arbitrary events here, even ones that are not pre-defined by Eggdrop. For example: callevent rehash, or callevent myownevent123.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        callevent example-event

----

.. _tcl-traffic:

^^^^^^^
traffic
^^^^^^^

  Description
     Returns a list of sublists containing information about the bot's traffic usage in bytes. Each sublist contains five elements: type, in-traffic today, in-traffic total, out-traffic today, out-traffic total (in that order).

  Returns
     a list of sublists containing information about the bot's traffic usage in bytes. Each sublist contains five elements: ``type``, ``in-traffic today``, ``in-traffic total``, ``out-traffic today``, ``out-traffic total`` (in that order).


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set usage [traffic]

----

.. _tcl-modules:

^^^^^^^
modules
^^^^^^^

  Description
     Returns a list of sublists containing information about the bot's currently loaded modules. Each sublist contains three elements: module, version, and dependencies. Each dependency is also a sublist containing the module name and version.
  Returns
     a list of sublists containing information about the bot's currently loaded modules. Each sublist contains three elements: ``module``, ``version``, and ``dependencies``. Each dependency is also a sublist containing the module name and version.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set loaded_modules [modules]

----

.. _tcl-loadmodule:

^^^^^^^^^^^^^^^^^^^
loadmodule <module>
^^^^^^^^^^^^^^^^^^^

  Description
     Attempts to load the specified module.

  Returns
     ``Already loaded.`` if the module is already loaded, ``""`` if successful, or the reason the module couldn't be loaded.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [loadmodule notes]

----

.. _tcl-unloadmodule:

^^^^^^^^^^^^^^^^^^^^^
unloadmodule <module>
^^^^^^^^^^^^^^^^^^^^^

  Description
     Attempts to unload the specified module.

  Returns
     ``No such module`` if the module is not loaded, ``""`` otherwise.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set result [unloadmodule notes]

----

.. _tcl-loadhelp:

^^^^^^^^^^^^^^^^^^^^^^^^
loadhelp <helpfile-name>
^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Attempts to load the specified help file from the help/ directory.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        loadhelp example.help

----

.. _tcl-unloadhelp:

^^^^^^^^^^^^^^^^^^^^^^^^^^
unloadhelp <helpfile-name>
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Attempts to unload the specified help file.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        unloadhelp example.help

----

.. _tcl-reloadhelp:

^^^^^^^^^^
reloadhelp
^^^^^^^^^^

  Description
     Reloads the bot's help files.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        reloadhelp

----

.. _tcl-restart:

^^^^^^^
restart
^^^^^^^

  Description
     Rehashes the bot, kills all timers, reloads all modules, and reconnects the bot to the next server in its list.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        restart

----

.. _tcl-rehash:

^^^^^^
rehash
^^^^^^

  Description
     Rehashes the bot.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        rehash

----

.. _tcl-stripcodes:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
stripcodes <strip-flags> <string>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Strips specified control characters from the string given. strip-flags can be any combination of the following:

     .. list-table::
        :widths: 8 92
        :header-rows: 1

        * - ``strip-flags``
          - Description
        * - ``c``
          - remove all color codes
        * - ``b``
          - remove all boldface codes
        * - ``r``
          - remove all reverse video codes
        * - ``u``
          - remove all underline codes
        * - ``a``
          - remove all ANSI codes
        * - ``g``
          - remove all ctrl-g (bell) codes
        * - ``o``
          - remove all ordinary codes (ctrl+o, terminates bold/color/..)
        * - ``i``
          - remove all italics codes
        * - ``\*``
          - remove all of the above

  Returns
     the stripped string.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set clean [stripcodes c "Example text"]

----

.. _tcl-matchaddr:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchaddr <hostmask> <address>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Checks if the address matches the hostmask given. The address should be in the form nick!user\@host.

  Returns
     ``1`` if the address matches the hostmask, ``0`` otherwise.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set matched [matchaddr "*!*@127.0.0.1" "Foobar!foobar@127.0.0.1"]

----

.. _tcl-matchcidr:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchcidr <block> <address> <prefix>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Performs a cidr match on the specified ip addresses. IPv6 is supported, if enabled at compile time.


  Returns
     ``1`` if the address matches the block prefix, ``0`` otherwise.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set matched [matchcidr 127.0.0.0 127.0.0.1 8]

----

.. _tcl-matchstr:

^^^^^^^^^^^^^^^^^^^^^^^^^^^
matchstr <pattern> <string>
^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Checks if pattern matches string. Only two wildcards are supported: ``*`` and '?'. Matching is case-insensitive. This command is intended as a simplified alternative to Tcl's string match.

  Returns
     ``1`` if the pattern matches the string, ``0`` if it doesn't.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set matched [matchstr "Foo*" "Foobar"]

----

.. _tcl-rfcequal:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^
rfcequal <string1> <string2>
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  Description
     Checks if two strings are equal. Case is ignored, and this uses RFC1459 matching {}|~ == []\^, depending on the rfc_compliant setting.

  Returns
     ``1`` if equal, ``0`` if not.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set equal [rfcequal "Foobar" "foobar"]

----

.. _tcl-status:

^^^^^^^^^^^^^
status [type]
^^^^^^^^^^^^^

  Description
     Provides eggdrop status information similar to the ``.status`` command in partyline. The available types of information are:

     .. list-table::
        :widths: 8 92
        :header-rows: 1

        * - ``type``
          - Description
        * - ``cpu``
          - total cpu time spent by eggdrop
        * - ``mem``
          - dynamically allocated memory excluding the Tcl interpreter
        * - ``cache``
          - user entries cache hits (in %)
        * - ``ipv6``
          - shows whether IPv6 support was compiled in

  Returns
     the requested information type or all information if type isn't specified. The format is a flat list of name-value pairs.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set cpu_info [status cpu]

----

.. _tcl-istls:

^^^^^^^^^^^
istls <idx>
^^^^^^^^^^^

  Description
     Checks if a connection is encrypted or cleartext. This command is available on TLS-enabled bots only.

  Returns
     ``1`` if the idx is a TLS connection, ``0`` if it's plaintext.


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set secure [istls 0]

----

.. _tcl-starttls:

^^^^^^^^^^^^^^
starttls <idx>
^^^^^^^^^^^^^^

  Description
     Establishes a secure (using TLS) connection over idx. The TLS connection should be first negotiated over the plaintext link, or using other means. Both parties must switch to TLS simultaneously. This command is available on TLS-enabled bots only.

  Returns
     nothing


  Module
     ``core``


  .. admonition:: Example — Start TLS on a connection
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        starttls 0

----

.. _tcl-tlsstatus:

^^^^^^^^^^^^^^^
tlsstatus <idx>
^^^^^^^^^^^^^^^

  Description
     Provides information about an established TLS connection This includes certificate and cipher information as well as protocol version. This command is available on TLS-enabled bots only.

  Returns
     a flat list of name-value pairs


  Module
     ``core``


  .. admonition:: Example
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        set details [tlsstatus 0]

Global Variables
----------------

NOTE: All config file variables are also global.

^^^^^^^
botnick
^^^^^^^

  Value
     the current nickname the bot is using (for example: "LamestBot", "LamestBot1", etc.)

  Module
     ``server``

^^^^^^^
botname
^^^^^^^

  Value
     the current nick!user\@host that the server sees (for example: "LamestBot!eggdrop\@127.0.0.1")

  Module
     ``server``

^^^^^^
server
^^^^^^

  Value
     the current server's real name (what server calls itself) and port bot is connected to (for example: "irc.example.net:6667") Note that this does not necessarily match the servers internet address.

  Module
     ``server``

^^^^^^^^^^^^^
serveraddress
^^^^^^^^^^^^^

  Value
     the current server's internet address (hostname or IP) and port bot is connected to. This will correspond to the entry in server list (for example: "irc.example.net:6667"). Note that this does not necessarily match the name server calls itself.

  Module
     ``server``

^^^^^^^
version
^^^^^^^

  Value
     current bot version "1.1.2+pl1.10.2201"; first item is the text version, to include a patch string if present, and second item is a numerical version

  Module
     ``core``

^^^^^^^^^^^
numversion*
^^^^^^^^^^^

  Value
     the current numeric bot version (for example: "1.10.21"). Numerical version is in the format of "MNNRRPP", where:

     .. list-table::
        :widths: 13 87
        :header-rows: 1

        * - MNNRRPP component
          - Description
        * - ``M``
          - major release number
        * - ``NN``
          - minor release number
        * - ``RR``
          - sub-release number
        * - ``PP``
          - patch level for that sub-release

  Module
     ``core``

^^^^^^
uptime
^^^^^^

  Value
     the unixtime value for when the bot was started

  Module
     ``core``

^^^^^^^^^^^^^
server-online
^^^^^^^^^^^^^

  Value
     the unixtime value when the bot connected to its current server, or '0' if the bot is currently disconnected from a server.

  Module
     ``server``

^^^^^^^^
lastbind
^^^^^^^^

  Value
     the last command binding which was triggered. This allows you to identify which command triggered a Tcl proc.

  Module
     ``core``

^^^^^^^
isjuped
^^^^^^^

  Value
     1 if bot's nick is juped(437); 0 otherwise

  Module
     ``server``

^^^^^^^
handlen
^^^^^^^

  Value
     the value of the HANDLEN define in src/eggdrop.h

  Module
     ``core``

^^^^^^
config
^^^^^^

  Value
     the filename of the config file Eggdrop is currently using

  Module
     ``core``

^^^^^^^^^^^^^
configureargs
^^^^^^^^^^^^^

  Value
     a string (not list) of configure arguments in shell expansion (single quotes)

  Module
     ``core``

^^^^^^^^
language
^^^^^^^^

  Value
     a string containing the language with the highest priority for use by Eggdrop. This commonly reflects what is added with addlang in the config file

  Module
     ``core``

^^^^^^^^^^^^^^
account-extban
^^^^^^^^^^^^^^

  Value
     a string containing the value of ACCOUNTEXTBAN provided by the 005 connection message. This is the raw value and, depending on the IRC server formatting, could be in multiple formats such as "a", or "a,account".

  Module
     ``channel``

extban-flags
^^^^^^^^^^^^
  Value
     a string containing the allowed extban flag characters advertised by EXTBAN in the 005 connection message. If EXTBAN advertises a prefix, such as "$,aUq", the prefix is omitted and this variable contains only the flags, such as "aUq".

  Module
     ``channel``


Binds
-----

You can use the 'bind' command to attach Tcl procedures to certain events. For example, you can write a Tcl procedure that gets called every time a user says "danger" on the channel. When a bind is triggered, ALL of the Tcl procs that are bound to it will be called. Raw binds are triggered before builtin binds, as a builtin bind has the potential to modify args.

^^^^^^^^^^^^^^^
Stackable binds
^^^^^^^^^^^^^^^

Some bind types are marked as "stackable". That means that you can bind multiple commands to the same trigger. Normally, for example, a bind such as 'bind msg - stop msg:stop' (which makes a msg-command "stop" call the Tcl proc "msg:stop") will overwrite any previous binding you had for the msg command "stop". With stackable bindings, like 'msgm' for example, you can bind the same command to multiple procs.

^^^^^^^^^^^^^^^
Removing a bind
^^^^^^^^^^^^^^^

To remove a bind, use the 'unbind' command. For example, to remove the
bind for the "stop" msg command, use 'unbind msg - stop msg:stop'.

.. _tcl_binds:

^^^^^^^^^^
Flag Masks
^^^^^^^^^^

In the `Bind Types`_ section (and other commands, such as `matchattr`_), you will see several references to the "flags" argument. The "flags" argument takes a flag mask, which is a value that represents the type of user that is allowed to trigger the procedure associated to that bind. The flags can be any of the standard Eggdrop flags (o, m, v, etc). Additionally, when used by itself, a "-" or "*" can be used to skip processing for a flag type. A flag mask has three sections to it- global, channel, and bot flag sections. Each section is separated by the | or & logical operators ( the | means "OR" and the & means "AND; if nothing proceeds the flag then Eggdrop assumes it to be an OR). Additionally, a '+' and '-' can be used in front of a flag to check if the user does (+) have it, or does not (-) have it.

The easiest way to explain how to build a flag mask is by demonstration. A flag mask of "v" by itself means "has a global v flag". To also check for a channel flag, you would use the flag mask "v\|v". This checks if the user has a global "v" flag, OR a channel "v" flag (again, the | means "OR" and ties the two types of flags together). You could change this mask to be "v&v", which would check if the user has a global "v" flag AND a channel "v" flag. Lastly, to check if a user ONLY has a channel flag, you would use "\*|v" as a mask, which would not check global flags but does check if the user had a channel "v" flag.

You will commonly see flag masks for global flags written "ov"; this is the same as "\|ov" or "\*\|ov".

Some additional examples:

.. list-table::
   :widths: 14 86
   :header-rows: 1

   * - Flag Mask
     - Description
   * - ``m``, ``+m``, ``m|*``
     - Checks if the user has the m global flag
   * - ``+mn``
     - Checks if the user has the m OR n global flag
   * - ``\\|+mn``
     - Checks if the user has the m OR n channel flag
   * - \\|+mn #lamest
     - Checks if the user has the m OR n channel flag for #lamest
   * - ``&+mn``
     - Checks if the user has the m AND n channel flag
   * - ``&mn #lamest``
     - Checks if the user has the m AND n channel flag for #lamest
   * - \\|+o #lamest
     - Checks if the user has the o channel flag for #lamest
   * - ``+o|+n #lamest``
     - Checks if the user has the o global flag OR the n channel flag for #lamest
   * - ``+m&+v #lamest``
     - Checks if the user has the m global flag AND the v channel flag for #lamest
   * - ``-m``
     - Checks if the user does not have the m global flag
   * - \\|-n #lamest
     - Checks if the user does not have the n channel flag for #lamest
   * - ``+m|-n #lamest``
     - Checks if the user has the global m flag OR does not have a channel n flag for #lamest
   * - ``-n&-m #lamest``
     - Checks if the user does not have the global n flag AND does not have the channel m flag for #lamest
   * - ``||+b``
     - Checks if the user has the bot flag b

As a side note, Tcl scripts historically have used a '-' to skip processing of a flag type (Example: -\|o). It is unknown where and why this practice started, but as a style tip, Eggdrop developers recommend using a '\*' to skip processing, so as not to confuse a single "-" meaning "skip processing" with a preceding "-ov" which means "not these flags".

.. _bind_types:

^^^^^^^^^^
Bind Types
^^^^^^^^^^

The following is a list of bind types and how they work. Below each bind type is the format of the bind command, the list of arguments sent to the Tcl proc, and an explanation.

.. _bind-msg:

(1)  MSG

  ``bind msg <flags> <command> <proc>``

  ``procname <nick> <user@host> <handle> <text>``

  Description
     Used for ``/msg`` commands. The first word of the user's msg is the command, and everything else becomes the text argument.


  Module
     ``server``


  .. admonition:: Example — MSG trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind msg * !hello msg_proc
        
        proc msg_proc {nick user hand text} {
            putlog "Nick is $nick, user is $user, handle is $hand, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        /msg LamestBot !hello world
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, text is world

.. _bind-dcc:

(2)  DCC

  ``bind dcc <flags> <command> <proc>``

  ``procname <handle> <idx> <text>``

  Description
     Used for partyline commands; the command is the first word and everything else is the text argument. The idx is valid until the user disconnects. After that, it may be reused, so be careful about storing an idx for long periods of time.


  Module
     ``core``


  .. admonition:: Example — DCC trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind dcc * hello dcc_proc
        
        proc dcc_proc {hand idx text} {
            putlog "Handle is $hand, idx is $idx, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        <foobar> .hello world
        <LamestBot> Handle is foobar, idx is 3, text is world

.. _bind-fil:

(3)  FIL

  ``bind fil <flags> <command> <proc>``

  ``procname <handle> <idx> <text>``

  Description
     The same as DCC, except this is triggered if the user is in the file area instead of the party line.


  Module
     ``filesys``


  .. admonition:: Example — FIL trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind fil * hello fil_proc
        
        proc fil_proc {hand idx text} {
            putlog "Handle is $hand, idx is $idx, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        <foobar> .hello world
        <LamestBot> Handle is foobar, idx is 3, text is world

.. _bind-pub:

(4)  PUB

  ``bind pub <flags> <command> <proc>``

  ``procname <nick> <user@host> <handle> <channel> <text>``

  Description
     Used for commands given on a channel. The first word becomes the command and everything else is the text argument.


  Module
     ``irc``


  .. admonition:: Example — PUB trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind pub * !hello pub_proc
        
        proc pub_proc {nick user hand chan text} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        <Foobar> !hello world
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest, text is world

.. _bind-msgm:

(5)  MSGM (stackable)

  ``bind msgm <flags> <mask> <proc>``

  ``procname <nick> <user@host> <handle> <text>``

  Description
     Matches the entire line of text from a ``/msg`` with the mask. This is useful for binding Tcl procs to words or phrases spoken anywhere within a line of text. If the proc returns ``1``, Eggdrop will not log the message that triggered this bind. MSGM binds are processed before MSG binds. If the exclusive-binds setting is enabled, MSG binds will not be triggered by text that a MSGM bind has already handled.


  Module
     ``server``


  .. admonition:: Example — MSGM trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind msgm * "*" msgm_proc
        
        proc msgm_proc {nick user hand text} {
            putlog "Nick is $nick, user is $user, handle is $hand, text is $text"
            return 0
        }

     **Trigger and output**

     .. code-block:: text

        /msg LamestBot hello world
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, text is hello world

.. _bind-pubm:

(6)  PUBM (stackable)

  ``bind pubm <flags> <mask> <proc>``

  ``procname <nick> <user@host> <handle> <channel> <text>``

  Description
     Just like MSGM, except it's triggered by things said on a channel instead of things ``/msg``'d to the bot. The mask is matched against the channel name followed by the text and can contain wildcards. If the proc returns ``1``, Eggdrop will not log the message that triggered this bind. PUBM binds are processed before PUB binds. If the exclusive-binds setting is enabled, PUB binds will not be triggered by text that a PUBM bind has already handled.


  Module
     ``irc``


  .. admonition:: Example — PUBM trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind pubm * "% *" pubm_proc
        
        proc pubm_proc {nick user hand chan text} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, text is $text"
            return 0
        }

     **Trigger and output**

     .. code-block:: text

        <Foobar> hello world
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest, text is hello world

.. _bind-notc:

(7)  NOTC (stackable)

  ``bind notc <flags> <mask> <proc>``

  ``procname <nick> <user@host> <handle> <text> <dest>``

  Description
     Dest will be a nickname (the bot's nickname, obviously) or a channel name. mask is matched against the entire text of the notice and can contain wildcards. It is considered a breach of protocol to respond to a ``/notice`` on IRC, so this is intended for internal use (logging, etc.) only. Note that server notices do not trigger the NOTC bind. If the proc returns ``1``, Eggdrop will not log the message that triggered this bind.

     New Tcl procs should be declared as::

      proc notcproc {nick uhost hand text {dest ""}} {
        global botnick; if {$dest == ""} {set dest $botnick}
        ...
      }

     for compatibility.


  Module
     ``server``


  .. admonition:: Example — NOTC trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind notc * "*" notc_proc
        
        proc notc_proc {nick user hand text {dest ""}} {
            putlog "Nick is $nick, user is $user, handle is $hand, text is $text, destination is $dest"
            return 0
        }

     **Trigger and output**

     .. code-block:: text

        Foobar sends NOTICE LamestBot :hello world
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, text is hello world, destination is LamestBot

.. _bind-join:

(8)  JOIN (stackable)

  ``bind join <flags> <mask> <proc>``

  ``procname <nick> <user@host> <handle> <channel>``

  Description
     Triggered by someone joining the channel. The mask in the bind is matched against ``#channel nick!user\@host`` and can contain wildcards.


  Module
     ``irc``


  .. admonition:: Example — JOIN trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind join * "#lamest *" join_proc
        
        proc join_proc {nick user hand chan} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar joins #lamest
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest

.. _bind-part:

(9)  PART (stackable)

  ``bind part <flags> <mask> <proc>``

  ``procname <nick> <user@host> <handle> <channel> <msg>``

  Description
     Triggered by someone leaving the channel. The mask is matched against ``#channel nick!user\@host`` and can contain wildcards. If no part message is specified, msg will be set to ``""``.

     New Tcl procs should be declared as::

       proc partproc {nick uhost hand chan {msg ""}} { ... }

     for compatibility.


  Module
     ``irc``


  .. admonition:: Example — PART trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind part * "#lamest *" part_proc
        
        proc part_proc {nick user hand chan {msg ""}} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, message is $msg"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar parts #lamest with "Goodbye"
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest, message is Goodbye

.. _bind-sign:

(10) SIGN (stackable)

  ``bind sign <flags> <mask> <proc>``
  
  ``procname <nick> <user@host> <handle> <channel> <reason>``

  Description
     Triggered by a signoff, or possibly by someone who got netsplit and never returned. The signoff message is the last argument to the proc. Wildcards can be used in the mask, which is matched against ``#channel nick!user\@host``. If a ``*`` is used for the channel in the mask, this bind is triggered once for every channel that the user is in the bot with; in other words if the bot is in two channels with the target user, the bind will be triggered twice. To trigger a proc only once per signoff, regardless of the number of channels the Eggdrop and user share, use the RAWT bind with SIGN as the keyword.


  Module
     ``irc``


  .. admonition:: Example — SIGN trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind sign * "#lamest *" sign_proc
        
        proc sign_proc {nick user hand chan reason} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, reason is $reason"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar quits IRC with "Leaving"
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest, reason is Leaving

.. _bind-topc:

(11) TOPC (stackable)

  ``bind topc <flags> <mask> <proc>``
  
  ``procname <nick> <user@host> <handle> <channel> <topic>``

  Description
     Triggered by a topic change. mask can contain wildcards and is matched against ``#channel <new topic>``.


  Module
     ``irc``


  .. admonition:: Example — TOPC trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind topc * "#lamest *" topc_proc
        
        proc topc_proc {nick user hand chan topic} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, topic is $topic"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar changes the #lamest topic to "Example topic"
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest, topic is Example topic

.. _bind-kick:

(12) KICK (stackable)

  ``bind kick <flags> <mask> <proc>``
  
  ``procname <nick> <user@host> <handle> <channel> <target> <reason>``

  Description
     Triggered when someone is kicked off the channel. The mask is matched against ``#channel target reason`` where the target is the nickname of the person who got kicked (can contain wildcards). The proc is called with the nick, user\@host, and handle of the kicker, plus the channel, the nickname of the person who was kicked, and the reason.


  Module
     ``irc``


  .. admonition:: Example — KICK trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind kick * "#lamest *" kick_proc
        
        proc kick_proc {nick user hand chan target reason} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, target is $target, reason is $reason"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar kicks Guest from #lamest with "Example reason"
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest, target is Guest, reason is Example reason

.. _bind-nick:

(13) NICK (stackable)

  ``bind nick <flags> <mask> <proc>``
  
  ``procname <nick> <user@host> <handle> <channel> <newnick>``

  Description
     Triggered when someone changes nicknames. The mask is matched against ``#channel newnick`` and can contain wildcards. Channel is ``*`` if the user isn't on a channel (usually the bot not yet in a channel). If a ``*`` is used for the channel in the mask, this bind is triggered once for every channel that the user is in the bot with; in other words if the bot is in two channels with the target user, the bind will be triggered twice. To trigger a proc only once per nick change, regardless of the number of channels the Eggdrop and user share, use the RAWT bind with NICK as the keyword.


  Module
     ``irc``


  .. admonition:: Example — NICK trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind nick * "#lamest *" nick_proc
        
        proc nick_proc {nick user hand chan newnick} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, new nick is $newnick"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar changes nick to Foobar2
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest, new nick is Foobar2

.. _bind-mode:

(14) MODE (stackable)

  ``bind mode <flags> <mask> <proc>``

  ``procname <nick> <user@host> <handle> <channel> <mode-change> <target>``

  Description
     Mode changes are broken down into their component parts before being sent here, so the ``<mode-change>`` will always be a single mode, such as ``+m`` or ``-o``. target will show the argument of the mode change (for o/v/b/e/I) or ``""`` if the set mode does not take an argument. The bot's automatic response to a mode change will happen AFTER all matching Tcl procs are called. The mask will be matched against ``#channel +/-modes`` and can contain wildcards.

     If it is a server mode, nick will be ``""``, user\@host is the server name, and handle is \*.
      
     Note that ``target`` was added in 1.3.17 and that this will break Tcl scripts that were written for pre-1.3.17 Eggdrop that use the mode binding. Also, due to a typo, mode binds were broken completely in 1.3.17 but were fixed in 1.3.18. Mode bindings are not triggered at all in 1.3.17.

     One easy example (from guppy) of how to support the ``target`` parameter in 1.3.18 and later and still remain compatible with older Eggdrop versions is:

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


  Module
     ``irc``


  .. admonition:: Example — MODE trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind mode * "#lamest *" mode_proc
        
        proc mode_proc {nick user hand chan mode target} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, mode is $mode, target is $target"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar sets +o Guest on #lamest
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest, mode is +o, target is Guest

.. _bind-ctcp:

(15) CTCP (stackable)

  ``bind ctcp <flags> <keyword> <proc>``
  ``procname <nick> <user@host> <handle> <dest> <keyword> <text>``

  Description
     Dest will be a nickname (the bot's nickname, obviously) or channel name. keyword is the ctcp command (which can contain wildcards), and text may be empty. If the proc returns ``0``, the bot will attempt its own processing of the ctcp command.


  Module
     ``server``


  .. admonition:: Example — CTCP trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind ctcp * VERSION ctcp_proc
        
        proc ctcp_proc {nick user hand dest keyword text} {
            putlog "Nick is $nick, user is $user, handle is $hand, destination is $dest, keyword is $keyword, text is $text"
            return 0
        }

     **Trigger and output**

     .. code-block:: text

        Foobar sends a CTCP VERSION request to LamestBot
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, destination is LamestBot, keyword is VERSION, text is

.. _bind-ctcr:

(16) CTCR (stackable)

  ``bind ctcr <flags> <keyword> <proc>``

  ``procname <nick> <user@host> <handle> <dest> <keyword> <text>``

  Description
     Just like ctcp, but this is triggered for a ctcp-reply (ctcp embedded in a notice instead of a privmsg).


  Module
     ``server``


  .. admonition:: Example — CTCR trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind ctcr * VERSION ctcr_proc
        
        proc ctcr_proc {nick user hand dest keyword text} {
            putlog "Nick is $nick, user is $user, handle is $hand, destination is $dest, keyword is $keyword, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar sends a CTCP VERSION reply to LamestBot
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, destination is LamestBot, keyword is VERSION, text is Example client

.. _bind-raw:

(17) RAW (stackable)

  ``bind raw <flags> <mask> <proc>``

  ``procname <from> <keyword> <text>``

  IMPORTANT: While not necessarily deprecated, this bind has been supplanted by the RAWT bind, which supports the IRCv3 message-tags capability, as of 1.9.0. You probably want to be using RAWT, not RAW.

  Description
     The mask can contain wildcards and is matched against the keyword, which is either a numeric, like ``368``, or a keyword, such as ``PRIVMSG``. ``from`` will be the server name or the source nick!ident@host (depending on the keyword); flags are ignored. If the proc returns ``1``, Eggdrop will not process the line any further (this could cause unexpected behavior in some cases), although RAWT binds are processed before RAW binds (and thus, a RAW bind cannot block a RAWT bind).


  Module
     ``server``


  .. admonition:: Example — RAW trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind raw - PRIVMSG raw_proc
        
        proc raw_proc {from keyword text} {
            putlog "From is $from, keyword is $keyword, text is $text"
            return 0
        }

     **Trigger and output**

     .. code-block:: text

        Foobar sends PRIVMSG LamestBot :hello world
        <LamestBot> From is Foobar!foobar@127.0.0.1, keyword is PRIVMSG, text is LamestBot :hello world

.. _bind-bot:

(18) BOT

  ``bind bot <flags> <command> <proc>``

  ``procname <from-bot> <command> <text>``

  Description
     Triggered by a message coming from another bot in the botnet. The first word is the command and the rest becomes the text argument; flags are ignored.


  Module
     ``core``


  .. admonition:: Example — BOT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind bot - HELLO bot_proc
        
        proc bot_proc {from command text} {
            putlog "From bot is $from, command is $command, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        RemoteBot sends botnet message "HELLO hello world"
        <LamestBot> From bot is RemoteBot, command is HELLO, text is hello world

.. _bind-chon:

(19) CHON (stackable)

  ``bind chon <flags> <mask> <proc>``

  ``procname <handle> <idx>``

  Description
     When someone first enters the party-line area of the bot via dcc chat or telnet, this is triggered before they are connected to a chat channel (so, yes, you can change the channel in a ``chon`` proc). mask is matched against the handle and supports wildcards. This is NOT triggered when someone returns from the file area, etc.


  Module
     ``core``


  .. admonition:: Example — CHON trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind chon * "*" chon_proc
        
        proc chon_proc {hand idx} {
            putlog "Handle is $hand, idx is $idx"
        }

     **Trigger and output**

     .. code-block:: text

        foobar enters the party line
        <LamestBot> Handle is foobar, idx is 3

.. _bind-chof:

(20) CHOF (stackable)

  ``bind chof <flags> <mask> <proc>``

  ``procname <handle> <idx>``

  Description
     Triggered when someone leaves the party line to disconnect from the bot. mask is matched against the handle and can contain wildcards. Note that the connection may have already been dropped by the user, so don't send output to the idx.


  Module
     ``core``


  .. admonition:: Example — CHOF trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind chof * "*" chof_proc
        
        proc chof_proc {hand idx} {
            putlog "Handle is $hand, idx is $idx"
        }

     **Trigger and output**

     .. code-block:: text

        foobar leaves the party line
        <LamestBot> Handle is foobar, idx is 3

.. _bind-sent:

(21) SENT (stackable)

  ``bind sent <flags> <mask> <proc>``

  ``procname <handle> <nick> <path/to/file>``

  Description
     After a user has successfully downloaded a file from the bot, this binding is triggered. mask is matched against the handle of the user that initiated the transfer and supports wildcards. nick is the actual recipient (on IRC) of the file. The path is relative to the dcc directory (unless the file transfer was started by a script call to ``dccsend``, in which case the path is the exact path given in the call to ``dccsend``).


  Module
     ``transfer``


  .. admonition:: Example — SENT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind sent * "*" sent_proc
        
        proc sent_proc {hand nick path} {
            putlog "Handle is $hand, nick is $nick, path is $path"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar successfully downloads example.txt
        <LamestBot> Handle is foobar, nick is Foobar, path is example.txt

.. _bind-rcvd:

(22) RCVD (stackable)

  ``bind rcvd <flags> <mask> <proc>``

  ``procname <handle> <nick> <path/to/file>``

  Description
     Triggered after a user uploads a file successfully. mask is matched against the user's handle. nick is the IRC nickname that the file transfer originated from. The path is where the file ended up, relative to the dcc directory (usually this is your incoming dir).


  Module
     ``transfer``


  .. admonition:: Example — RCVD trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind rcvd * "*" rcvd_proc
        
        proc rcvd_proc {hand nick path} {
            putlog "Handle is $hand, nick is $nick, path is $path"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar successfully uploads example.txt
        <LamestBot> Handle is foobar, nick is Foobar, path is incoming/example.txt

.. _bind-chat:

(23) CHAT (stackable)

  ``bind chat <flags> <mask> <proc>``

  ``procname <handle> <channel#> <text>``

  Description
     When a user says something on the botnet, it invokes this binding. Flags are ignored; handle could be a user on this bot ("DronePup") or on another bot (``Eden\@Wilde``) and therefore you can't rely on a local user record. The mask is checked against the entire line of text and supports wildcards. Eggdrop passes the partyline channel number the user spoke on to the proc in ``channel#``.

     NOTE: If a BOT says something on the botnet, the BCST bind is invoked instead.


  Module
     ``core``


  .. admonition:: Example — CHAT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind chat - "*" chat_proc
        
        proc chat_proc {hand chan text} {
            putlog "Handle is $hand, channel is $chan, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        <foobar> hello world
        <LamestBot> Handle is foobar, channel is 0, text is hello world

.. _bind-link:

(24) LINK (stackable)

  ``bind link <flags> <mask> <proc>``

  ``procname <botname> <via>``

  Description
     Triggered when a bot links into the botnet. botname is the botnetnick of the bot that just linked in; via is the bot it linked through. The mask is checked against the botnetnick of the bot that linked and supports wildcards. flags are ignored.


  Module
     ``core``


  .. admonition:: Example — LINK trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind link - "*" link_proc
        
        proc link_proc {bot via} {
            putlog "Bot is $bot, via is $via"
        }

     **Trigger and output**

     .. code-block:: text

        RemoteBot links to the botnet through LamestBot
        <LamestBot> Bot is RemoteBot, via is LamestBot

.. _bind-disc:

(25) DISC (stackable)

  ``bind disc <flags> <mask> <proc>``

  ``procname <botname>``

  Description
     Triggered when a bot disconnects from the botnet for whatever reason. Just like the link bind, flags are ignored; mask is matched against the botnetnick of the bot that unlinked. Wildcards are supported in mask.


  Module
     ``core``


  .. admonition:: Example — DISC trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind disc - "*" disc_proc
        
        proc disc_proc {bot} {
            putlog "Bot is $bot"
        }

     **Trigger and output**

     .. code-block:: text

        RemoteBot disconnects from the botnet
        <LamestBot> Bot is RemoteBot

.. _bind-splt:

(26) SPLT (stackable)

  ``bind splt <flags> <mask> <proc>``

  ``procname <nick> <user@host> <handle> <channel>``

  Description
     Triggered when someone gets netsplit on the channel. Be aware that this may be a false alarm (it's easy to fake a netsplit signoff message on some networks); mask may contain wildcards and is matched against ``#channel nick!user\@host``. Anyone who is SPLT will trigger a REJN or SIGN within the next wait-split (defined in the config file) seconds.


  Module
     ``irc``


  .. admonition:: Example — SPLT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind splt * "#lamest *" splt_proc
        
        proc splt_proc {nick user hand chan} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar is detected as split from #lamest
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest

.. _bind-rejn:

(27) REJN (stackable)

  ``bind rejn <flags> <mask> <proc>``

  ``procname <nick> <user@host> <handle> <channel>``

  Description
     Someone who was split has rejoined. mask can contain wildcards, and is matched against ``#channel nick!user\@host``.


  Module
     ``irc``


  .. admonition:: Example — REJN trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind rejn * "#lamest *" rejn_proc
        
        proc rejn_proc {nick user hand chan} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar rejoins #lamest after a split
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, channel is #lamest

.. _bind-filt:

(28) FILT (stackable)

  ``bind filt <flags> <mask> <proc>``

  ``procname <idx> <text>``

  Description
     Party line and file system users have their text sent through filt before being processed. ``mask`` is a text mask that can contain wildcards and is used for matching text sent on the partyline. If the proc returns a blank string, the partyline texr is continued to be parsed as-is. Otherwise, the bot will instead use the text returned from the proc for continued parsing.


  Module
     ``core``


  .. admonition:: Example — FILT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind filt - "*" filt_proc
        
        proc filt_proc {idx text} {
            putlog "Idx is $idx, text is $text"
            return $text
        }

     **Trigger and output**

     .. code-block:: text

        <foobar> .who
        <LamestBot> Idx is 3, text is .who

.. _bind-need:

(29) NEED (stackable)

  ``bind need <flags> <mask> <proc>``

  ``procname <channel> <type>``

  Description
     This bind is triggered on certain events, like when the bot needs operator status or the key for a channel. The types are: op, unban, invite, limit, and key; the mask is matched against ``#channel type`` and can contain wildcards. flags are ignored.


  Module
     ``irc``


  .. admonition:: Example — NEED trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind need - "% op" need_proc
        
        proc need_proc {chan type} {
            putlog "Channel is $chan, type is $type"
        }

     **Trigger and output**

     .. code-block:: text

        LamestBot determines it needs operator status on #lamest
        <LamestBot> Channel is #lamest, type is op

.. _bind-flud:

(30) FLUD (stackable)

  ``bind flud <flags> <type> <proc>``

  ``procname <nick> <user@host> <handle> <type> <channel>``

  Description
     Any floods detected through the flood control settings (like ``flood-ctcp``) are sent here before processing. If the proc returns ``1``, no further action is taken on the flood; if the proc returns ``0``, the bot will do its normal ``punishment`` for the flood. The flood types are: pub, msg, join, or ctcp (and can be masked to ``*`` for the bind); flags are ignored.


  Module
     ``server``


  .. admonition:: Example — FLUD trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind flud - pub flud_proc
        
        proc flud_proc {nick user hand type chan} {
            putlog "Nick is $nick, user is $user, handle is $hand, type is $type, channel is $chan"
            return 0
        }

     **Trigger and output**

     .. code-block:: text

        Foobar triggers the configured public-message flood threshold on #lamest
        <LamestBot> Nick is Foobar, user is foobar@127.0.0.1, handle is foobar, type is pub, channel is #lamest

.. _bind-note:

(31) NOTE (stackable)

  ``bind note <flags> <mask> <proc>``

  ``procname <from> <to> <text>``

  Description
     Incoming notes (either from the party line, someone on IRC, or someone on another bot on the botnet) are checked against these binds before being processed. The mask is matched against the receiving handle and supports wildcards. If the proc returns ``1``, Eggdrop will not process the note any further. Flags are ignored.


  Module
     ``core``


  .. admonition:: Example — NOTE trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind note - foobar note_proc
        
        proc note_proc {from to text} {
            putlog "From is $from, to is $to, text is $text"
            return 0
        }

     **Trigger and output**

     .. code-block:: text

        OtherUser sends the note "hello world" to foobar
        <LamestBot> From is OtherUser, to is foobar, text is hello world

.. _bind-act:

(32) ACT (stackable)

  ``bind act <flags> <mask> <proc>``

  ``procname <handle> <channel#> <action>``

  Description
     When someone does an action on the botnet, it invokes this binding. flags are ignored; the mask is matched against the text of the action and can support wildcards.


  Module
     ``core``


  .. admonition:: Example — ACT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind act - "*" act_proc
        
        proc act_proc {hand chan action} {
            putlog "Handle is $hand, channel is $chan, action is $action"
        }

     **Trigger and output**

     .. code-block:: text

        foobar performs the partyline action "waves"
        <LamestBot> Handle is foobar, channel is 0, action is waves

.. _bind-wall:

(33) WALL (stackable)

  ``bind wall <flags> <mask> <proc>``

  ``procname <from> <msg>``

  Description
     When the bot receives a wallops, it invokes this binding. flags are ignored; the mask is matched against the text of the wallops msg. Note that RFC shows the server name as a source of the message, whereas many IRCds send the nick!user\@host of the actual sender, thus, Eggdrop will not parse it at all, but simply pass it to bind in its original form. If the proc returns ``1``,           Eggdrop will not log the message that triggered this bind.


  Module
     ``server``


  .. admonition:: Example — WALL trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind wall - "*" wall_proc
        
        proc wall_proc {from msg} {
            putlog "From is $from, message is $msg"
            return 0
        }

     **Trigger and output**

     .. code-block:: text

        irc.example.net sends WALLOPS :Example maintenance notice
        <LamestBot> From is irc.example.net, message is Example maintenance notice

.. _bind-bcst:

(34) BCST (stackable)

  ``bind bcst <flags> <mask> <proc>``
  ``procname <botname> <channel#> <text>``
 
  Description
     When a bot broadcasts something on the botnet (see ``dccbroadcast`` above), it invokes this binding. flags are ignored; the mask is matched against the message text and can contain wildcards. ``channel`` argument will always be ``-1`` since broadcasts are not directed to any partyline channel.
      
     It is also invoked when a BOT (not a person, as with the CHAT bind) ``says`` something on a channel. In this case, the ``channel`` argument will be a valid channel, and not ``-1``.


  Module
     ``core``


  .. admonition:: Example — BCST trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind bcst - "*" bcst_proc
        
        proc bcst_proc {bot chan text} {
            putlog "Bot is $bot, channel is $chan, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        RemoteBot broadcasts "hello botnet"
        <LamestBot> Bot is RemoteBot, channel is -1, text is hello botnet

.. _bind-chjn:

(35) CHJN (stackable)

  ``bind chjn <flags> <mask> <proc>``

  ``procname <botname> <handle> <channel#> <flag> <idx> <user@host>``

  Description
     When someone joins a botnet channel, it invokes this binding. The mask is matched against the channel and can contain wildcards. flag is one of: * (owner), + (master), @ (op), or % (botnet master). Flags are ignored.


  Module
     ``core``


  .. admonition:: Example — CHJN trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind chjn - "*" chjn_proc
        
        proc chjn_proc {bot hand chan flag idx user} {
            putlog "Bot is $bot, handle is $hand, channel is $chan, flag is $flag, idx is $idx, user is $user"
        }

     **Trigger and output**

     .. code-block:: text

        foobar joins botnet channel 0 on LamestBot
        <LamestBot> Bot is LamestBot, handle is foobar, channel is 0, flag is @, idx is 3, user is foobar@127.0.0.1

.. _bind-chpt:

(36) CHPT (stackable)

  ``bind chpt <flags> <mask> <proc>``

  ``procname <botname> <handle> <idx> <channel#>``

  Description
     When someone parts a botnet channel, it invokes this binding. The mask is matched against the channel and can contain wildcards. Flags are ignored.


  Module
     ``core``


  .. admonition:: Example — CHPT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind chpt - "*" chpt_proc
        
        proc chpt_proc {bot hand idx chan} {
            putlog "Bot is $bot, handle is $hand, idx is $idx, channel is $chan"
        }

     **Trigger and output**

     .. code-block:: text

        foobar leaves botnet channel 0 on LamestBot
        <LamestBot> Bot is LamestBot, handle is foobar, idx is 3, channel is 0

.. _bind-time:

(37) TIME (stackable)

  ``bind time <flags> <mask> <proc>``

  ``procname <minute 00-59> <hour 00-23> <day 01-31> <month 00-11> <year 0000-9999>``

  Description
     Allows you to schedule procedure calls at certain times. mask matches 5 space separated integers of the form: "minute hour day month year". The month var starts at 00 (Jan) and ends at 11 (Dec). Minute, hour, day, month have a zero padding so they are exactly two characters long; year is four characters. Flags are ignored.


  Module
     ``core``


  .. admonition:: Example — TIME trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind time - "* * * * *" time_proc
        
        proc time_proc {minute hour day month year} {
            putlog "Minute is $minute, hour is $hour, day is $day, month is $month, year is $year"
        }

     **Trigger and output**

     .. code-block:: text

        The clock reaches 12:34 on September 13, 2026
        <LamestBot> Minute is 34, hour is 12, day is 13, month is 08, year is 2026

.. _bind-away:

(38) AWAY (stackable)

  ``bind away <flags> <mask> <proc>``

  ``procname <botname> <idx> <text>``

  Description
     Triggers when a user goes away or comes back on the botnet. text is the reason that has been specified (text is ``""`` when returning). mask is matched against the botnet-nick of the bot the user is connected to and supports wildcards. flags are ignored.


  Module
     ``core``


  .. admonition:: Example — AWAY trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind away - "*" away_proc
        
        proc away_proc {bot idx text} {
            putlog "Bot is $bot, idx is $idx, text is $text"
        }

     **Trigger and output**

     .. code-block:: text

        foobar sets the partyline away message to "Lunch"
        <LamestBot> Bot is LamestBot, idx is 3, text is Lunch

.. _bind-load:

(39) LOAD (stackable)

  ``bind load <flags> <mask> <proc>``

  ``procname <module>``

  Description
     Triggers when a module is loaded. mask is matched against the name of the loaded module and supports wildcards; flags are ignored.


  Module
     ``core``


  .. admonition:: Example — LOAD trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind load - "*" load_proc
        
        proc load_proc {module} {
            putlog "Module is $module"
        }

     **Trigger and output**

     .. code-block:: text

        The irc module is loaded
        <LamestBot> Module is irc

.. _bind-unld:

(40) UNLD (stackable)

  ``bind unld <flags> <mask> <proc>``

  ``procname <module>``

  Description
     Triggers when a module is unloaded. mask is matched against the name of the unloaded module and supports wildcards;.
     flags are ignored.


  Module
     ``core``


  .. admonition:: Example — UNLD trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind unld - "*" unld_proc
        
        proc unld_proc {module} {
            putlog "Module is $module"
        }

     **Trigger and output**

     .. code-block:: text

        The irc module is unloaded
        <LamestBot> Module is irc

.. _bind-nkch:

(41) NKCH (stackable)

  ``bind nkch <flags> <mask> <proc>``

  ``procname <oldhandle> <newhandle>``

  Description
     Triggered whenever a local user's handle is changed (in the userfile). mask is matched against the user's old handle and can contain wildcards; flags are ignored.


  Module
     ``core``


  .. admonition:: Example — NKCH trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind nkch - "*" nkch_proc
        
        proc nkch_proc {oldhand newhand} {
            putlog "Old handle is $oldhand, new handle is $newhand"
        }

     **Trigger and output**

     .. code-block:: text

        The local handle foobar is changed to foobar2
        <LamestBot> Old handle is foobar, new handle is foobar2

.. _bind-evnt:

(42) EVNT (stackable)

  ``bind evnt <flags> <type> <proc>``

  ``procname <type> [arg]``

  Description
     Triggered whenever one of these events happen. flags are ignored. Pre-defined events triggered by Eggdrop are::

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
             hidden-host       - called after the bot's host is hidden by the server
             got-chanlist      - called after Eggdrop receives the channel userlist from the server. Passes a second [arg] value to the Tcl proc

     Note that Tcl scripts can trigger arbitrary events, including ones that are not pre-defined or used by Eggdrop.


  Module
     ``core``


  .. admonition:: Example — EVNT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind evnt - rehash evnt_proc
        
        proc evnt_proc {type {arg ""}} {
            putlog "Type is $type, arg is $arg"
        }

     **Trigger and output**

     .. code-block:: text

        A rehash event occurs
        <LamestBot> Type is rehash, arg is

.. _bind-lost:

(43) LOST (stackable)

  ``bind lost <flags> <mask> <proc>``

  ``procname <handle> <nick> <path> <bytes-transferred> <length-of-file>``

  Description
     Triggered when a DCC SEND transfer gets lost, such as when the connection is terminated before all data was successfully sent/received. This is typically caused by a user abort.


  Module
     ``transfer``


  .. admonition:: Example — LOST trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind lost - "*" lost_proc
        
        proc lost_proc {hand nick path bytes length} {
            putlog "Handle is $hand, nick is $nick, path is $path, bytes transferred is $bytes, file length is $length"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar aborts example.txt after 1024 of 4096 bytes
        <LamestBot> Handle is foobar, nick is Foobar, path is example.txt, bytes transferred is 1024, file length is 4096

.. _bind-tout:

(44) TOUT (stackable)

  ``bind tout <flags> <mask> <proc>``

  ``procname <handle> <nick> <path> <bytes-transferred> <length-of-file>``

  Description
     Triggered when a DCC SEND transfer times out. This may either happen because the DCC connection was not accepted or because the data transfer stalled for some reason.


  Module
     ``transfer``


  .. admonition:: Example — TOUT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind tout - "*" tout_proc

        proc tout_proc {hand nick path bytes length} {
            putlog "Handle is $hand, nick is $nick, path is $path, bytes transferred is $bytes, file length is $length"
        }

     **Trigger and output**

     .. code-block:: text

        A DCC SEND of example.txt to Foobar times out after 1024 of 4096 bytes
        <LamestBot> Handle is foobar, nick is Foobar, path is example.txt, bytes transferred is 1024, file length is 4096

.. _bind-out:

(45) OUT (stackable)

  ``bind out <flags> <mask> <proc>``

  ``procname <queue> <message> <queued|sent>``

  Description
     Triggered whenever output is sent to the server. Normally the event will occur twice for each line sent: once before entering a server queue and once after the message is actually sent. This allows for more flexible logging of server output and introduces the ability to cancel the message. Mask is matched against "queue status", where status is either ``queued`` or ``sent``. Queues are: mode, server, help, noqueue. noqueue is only used by the putnow Tcl command.


  Module
     ``server``


  .. admonition:: Example — OUT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind out - "server *" out_proc

        proc out_proc {queue message status} {
            putlog "Queue is $queue, message is $message, status is $status"
        }

     **Trigger and output**

     .. code-block:: text

        putserv "PRIVMSG #lamest :Hello, Foobar!"
        <LamestBot> Queue is server, message is PRIVMSG #lamest :Hello, Foobar!, status is queued
        <LamestBot> Queue is server, message is PRIVMSG #lamest :Hello, Foobar!, status is sent

.. _bind-cron:

(46) CRON (stackable)

  ``bind cron <flags> <mask> <proc>``

  ``procname <minute 0-59> <hour 0-23> <day 1-31> <month 1-12> <weekday 0-6>``

  Description
     Similar to bind TIME, but the mask is evaluated as a cron expression, e.g. ``16/2 */2 5-15 7,8,9 4``. It can contain up to five fields: minute, hour, day, month, weekday; delimited by whitespace. Week days are represented as 0-6, where Sunday can be either 0 or 7. Symbolic names are not supported. The bind will be triggered if the mask matches all of the fields, except that if both day and weekday are not ``*``, only one of them is required to match. If any number of fields are omitted at the end, the match will proceed as if they were ``*``. All cron operators are supported. Please refer to the crontab manual for their meanings. Flags are ignored.


  Module
     ``core``


  .. admonition:: Example — CRON trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind cron - "0 12 * * *" cron_proc

        proc cron_proc {minute hour day month weekday} {
            putlog "Minute is $minute, hour is $hour, day is $day, month is $month, weekday is $weekday"
        }

     **Trigger and output**

     .. code-block:: text

        The clock reaches 12:00 on September 13, 2026
        <LamestBot> Minute is 0, hour is 12, day is 13, month is 9, weekday is 0

.. _bind-log:

(47) LOG (stackable)

  ``bind log <flags> <mask> <proc>``

  ``procname <level> <channel> <message>``

  Description
     Triggered whenever a message is sent to a log. The mask is matched against "channel text". The level argument to the proc will contain the level(s) the message is sent to, or ``*`` if the message is sent to all log levels at once. If the message wasn't sent to a specific channel, channel will be set to ``*``.


  Module
     ``core``


  .. admonition:: Example — LOG trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind log - "#lamest *" log_proc

        proc log_proc {level channel message} {
            putlog "Level is $level, channel is $channel, message is $message"
        }

     **Trigger and output**

     .. code-block:: text

        A public message from Foobar is logged for #lamest
        <LamestBot> Level is p, channel is #lamest, message is <Foobar> Hello!

.. _bind-tls:

(48) TLS (stackable)

  ``bind tls <flags> <mask> <proc>``

  ``procname <idx>``

  Description
     Triggered for TCP connections when an SSL/TLS handshake has completed and the connection is secured. The mask is matched against the idx of the connection.


  Module
     ``core``


  .. admonition:: Example — TLS trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind tls - "*" tls_proc

        proc tls_proc {idx} {
            putlog "Idx is $idx"
        }

     **Trigger and output**

     .. code-block:: text

        TLS negotiation completes for connection idx 3
        <LamestBot> Idx is 3

.. _bind-die:

(49) DIE (stackable)

  ``bind die <flags> <mask> <proc>``

  ``procname <shutdownreason>``

  Description
     Triggered when Eggdrop is about to die. The mask is matched against the shutdown reason. The bind won't be triggered if the bot crashes or is being terminated by SIGKILL.


  Module
     ``core``


  .. admonition:: Example — DIE trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind die - "*" die_proc

        proc die_proc {reason} {
            putlog "Shutdown reason is $reason"
        }

     **Trigger and output**

     .. code-block:: text

        Eggdrop shuts down with the reason "Maintenance"
        <LamestBot> Shutdown reason is Maintenance

.. _bind-ircaway:

(50) IRCAWAY (stackable)

  ``bind ircaway <flags> <mask> <proc>``

  ``procname <nick> <user> <hand> <channel> <msg>``

  Description
     Triggered when Eggdrop receives an AWAY message for a user from an IRC server, ONLY if the away-notify capability is enabled via CAP (the server must support this capability; see the ``cap`` Tcl command for more information on requesting capabilities). "Normal" away messages (301 messages) will not trigger this bind; for those you should instead use a RAWT bind. The mask for the bind is in the format ``#channel nick!user@hostname`` (* to catch all nicknames). nick is the nickname of the user that triggered the bind, user is the nick!user@host of the user, handle is the handle of the user on the bot (- if the user is not added to the bot), channel is the channel the user was found on, and msg is the contents of the away message, if any. If a ``*`` is used for the channel in the mask, this bind is triggered once for every channel that the user is in with the bot. To trigger a proc only once per away change, regardless of the number of channels the Eggdrop and user share, use the RAWT bind with AWAY as the keyword.


  Module
     ``irc``


  .. admonition:: Example — IRCAWAY trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind ircaway - "#lamest *" ircaway_proc

        proc ircaway_proc {nick user hand channel msg} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $channel, message is $msg"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar sets an away message of "Lunch" while on #lamest
        <LamestBot> Nick is Foobar, user is Foobar!foobar@127.0.0.1, handle is foobar, channel is #lamest, message is Lunch

.. _bind-invt:

(51) INVT (stackable)

  ``bind invt <flags> <mask> <proc>``

  ``procname <nick> <user@host> <channel> <invitee>``

  Description
     Triggered when Eggdrop receives an INVITE message. The mask for the bind is in the format ``#channel nickname``, where nickname (not a hostmask) is that of the invitee. For the proc, nick is the nickname of the person sending the invite request, user@host is the user@host of the person sending the invite, channel is the channel the invitee is being invited to, and invitee is the target (nickname only) of the invite. The invitee argument was added to support the IRCv3 invite-notify capability, where Eggdrop may be able to see invite messages for other people that are not the Eggdrop.


  Module
     ``irc``


  .. admonition:: Example — INVT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind invt - "#lamest *" invt_proc

        proc invt_proc {nick userhost channel invitee} {
            putlog "Nick is $nick, user@host is $userhost, channel is $channel, invitee is $invitee"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar invites LamestBot to #lamest
        <LamestBot> Nick is Foobar, user@host is foobar@127.0.0.1, channel is #lamest, invitee is LamestBot

.. _bind-rawt:

(52) RAWT (stackable)

  ``bind rawt <flags> <mask> <proc>``

  ``procname <from> <keyword> <text> <tags>``

  Description
     Similar to the RAW bind, but allows an extra field for the IRCv3 message-tags capability. The mask can contain wildcards and is matched against the keyword which is either a numeric, like ``368``, or a keyword, such as ``PRIVMSG`` or ``TAGMSG``. from will be the server name or the source nick!ident@host (depending on the keyword); flags are ignored. tags is a dictionary (flat key/value list) of the message tags with ``""`` for empty values (e.g. "account eggdrop realname LamestBot"). If the proc returns ``1``, Eggdrop will not process the line any further, including processing by a RAW bind (this could cause unexpected behavior in some cases). As of 1.9.0, it is recommended to use the RAWT bind instead of the RAW bind.


  Module
     ``server``


  .. admonition:: Example — RAWT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind rawt - "PRIVMSG" rawt_proc

        proc rawt_proc {from keyword text tags} {
            putlog "From is $from, keyword is $keyword, text is $text, tags are $tags"
        }

     **Trigger and output**

     .. code-block:: text

        @account=foobar :Foobar!foobar@127.0.0.1 PRIVMSG #lamest :Hello!
        <LamestBot> From is Foobar!foobar@127.0.0.1, keyword is PRIVMSG, text is #lamest :Hello!, tags are account foobar

.. _bind-account:

(53) ACCOUNT (stackable)

  ``bind account <flags> <mask> <proc>``

  ``procname <nick> <user> <hand> <chan> <account>``

  Description
     This bind will trigger when Eggdrop detects a change in the authentication status of a user's services account. The mask for the bind is in the format ``#channel nick!user@hostname.com account`` and accepts wildcards. account is either the account name the user is logging in to or ``*`` if the user is not logged in to an account.

     NOTE: The three required IRC components for account tracking are the WHOX feature, the extended-join IRCv3 capability, and the account-notify IRCv3 capability. If only some of the three features are available, Eggdrop provides best-effort account tracking but this bind could be triggered late or never on account changes. Please see ``doc/ACCOUNTS`` for additional information.


  Module
     ``irc``


  .. admonition:: Example — ACCOUNT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind account - "#lamest *" account_proc

        proc account_proc {nick user hand chan account} {
            putlog "Nick is $nick, user is $user, handle is $hand, channel is $chan, account is $account"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar identifies to services as foobar while on #lamest
        <LamestBot> Nick is Foobar, user is Foobar!foobar@127.0.0.1, handle is foobar, channel is #lamest, account is foobar

.. _bind-isupport:

(54) ISUPPORT (stackable)

  ``bind isupport <flags> <mask> <proc>``

  ``procname <key> <isset> <value>``

  Description
     Triggered when the value of an ISUPPORT key changes. The mask is matched against the ISUPPORT key. If the value is not set, isset is ``0`` and the value is the empty string. Because the empty string is a valid value, use isset to distinguish empty string values from a key being unset. The bind is called before the change is processed, so [isupport isset]/[isupport get] return the old value. A return value other than 0 makes Eggdrop ignore the change and revert to the old value. After a disconnect from the server, all ISUPPORT values are reset to default, but $::server will be empty, so that case can be caught and ignored.


  Module
     ``server``


  .. admonition:: Example — ISUPPORT trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind isupport - "CHANTYPES" isupport_proc

        proc isupport_proc {key isset value} {
            putlog "Key is $key, isset is $isset, value is $value"
        }

     **Trigger and output**

     .. code-block:: text

        The server advertises CHANTYPES=#&
        <LamestBot> Key is CHANTYPES, isset is 1, value is #&

.. _bind-monitor:

(55) MONITOR (stackable)

  ``bind monitor <flags> <nick> <proc>``

  ``procname <nick> <online>``

  Description
     Triggered when a server sends a MONITOR status change of a target either coming online or disconnecting (not all servers support MONITOR). flags are ignored, nick is the nickname of the intended MONITOR target and can be used with wildcards. For the proc, nick is the nickname connecting or disconnecting, and online is ``0`` if the nickname disconnected, or ``1`` if the nickname connected.


  Module
     ``irc``


  .. admonition:: Example — MONITOR trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind monitor - "Foobar" monitor_proc

        proc monitor_proc {nick online} {
            putlog "Nick is $nick, online is $online"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar connects to the IRC network
        <LamestBot> Nick is Foobar, online is 1

.. _bind-chghost:

(56) CHGHOST

  ``bind chghost <flags> <mask> <proc>``

  ``procname <nick> <old user@host> <handle> <channel> <new user@host>``

  Description
     Triggered when a server sends an IRCv3 CHGHOST message to change a user's hostmask. The new host is matched against mask in the form of ``#channel nick!user@host`` and can contain wildcards. The specified proc will be called with the nick of the user whose hostmask changed, the hostmask the affected user had before the change, the handle of the affected user (or * if no handle is present), the channel the user was on when the bind triggered, and the new hostmask of the affected user. This bind will trigger once for each channel the user is on.


  Module
     ``irc``


  .. admonition:: Example — CHGHOST trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind chghost - "#lamest *" chghost_proc

        proc chghost_proc {nick oldhost hand channel newhost} {
            putlog "Nick is $nick, old user@host is $oldhost, handle is $hand, channel is $channel, new user@host is $newhost"
        }

     **Trigger and output**

     .. code-block:: text

        Foobar changes host from foobar@127.0.0.1 to foobar@example.net on #lamest
        <LamestBot> Nick is Foobar, old user@host is foobar@127.0.0.1, handle is foobar, channel is #lamest, new user@host is foobar@example.net

.. _bind-chanset:

(57) CHANSET

  ``bind chanset <flags> <mask> <proc>``

  ``procname <chan> <setting> <value>``

  Description
     Triggered when a channel setting is set via the partyline. flags is ignored, mask is the name of channel setting (not including any +/- prefix) and can contain wildcards. The proc will be called with the channel that the setting was set on, the text name of the setting that was changed, and the value it was set to (0/1 for -/+, string, or X:Y formatted value).


  Module
     ``channels``


  .. admonition:: Example — CHANSET trigger
     :collapsible: closed
     :class: tcl-example

     .. code-block:: tcl

        bind chanset - "autoop" chanset_proc

        proc chanset_proc {chan setting value} {
            putlog "Channel is $chan, setting is $setting, value is $value"
        }

     **Trigger and output**

     .. code-block:: text

        <foobar> .chanset #lamest +autoop
        <LamestBot> Channel is #lamest, setting is autoop, value is 1

