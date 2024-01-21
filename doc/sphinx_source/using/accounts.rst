Account tracking in Eggdrop
===========================

In Eggdrop 1.9.3, Eggdrop added the ability to associate nicknames with the service accounts they are logged into. It is IMPORTANT to note that Eggdrop's ability to do this is dependent on an IRC server's implementation of three features- the IRCv3 extended-join capability, the IRCv3 account-notify capability, and WHOX support. All three of these features must be supported by the server and, in the case of extended-join and account-notify, requested by Eggdrop in order for Eggdrop to maintain "perfect" association between nicknames and account statuses.

Required Server Capabilities
----------------------------
You're going to see this repeated a lot- the IRC server must support three features in order for Eggdrop to accurately associate accounts with nicknames. These three features allow Eggdrop to always know the current association between an account and a nickname by getting account statuses of users already on a channel when it joins, new users joining a channel, and users who authenticate while on a channel.

extended-join
^^^^^^^^^^^^^
`extended-join <https://ircv3.net/specs/extensions/extended-join>`_ is an IRCv3-defined capability that adds the account name of a user to the JOIN message sent by the IRC server, alerting clients that a new member has joined a channel. Enabling this capability allows Eggdrop to immediately determine the account name associated with a user joining a channel

account-notify
^^^^^^^^^^^^^^
`account-notify <https://ircv3.net/specs/extensions/account-notify>`_ is an IRCv3-defined capability that sends a message to a channel when a member of the channel either authenticates or deauthenticates from their account. Enabling this capability allows Eggdrop to immediately associate an account to a channel member when they authenticate or deauthenticate.

WHOX
^^^^
'WHOX <https://ircv3.net/specs/extensions/whox>`_ is a server feature that allows a client to request custom fields to be returned in a WHO response. If a server supports this capability, Eggdrop sends a WHOX query to the server when it joins a channel, allowing it to immediately determine accounts associated with channel members when Eggdrop joins a channel.

Enabling Eggdrop Account Tracking
---------------------------------
By default, the Eggdrop config file will attempt to enable all the capabilities required for account tracking. There are two settings to pay attention to
::

  # To request the account-notify feature via CAP, set this to 1
  set account-notify 1

  # To request the extended-join feature via CAP, set this to 1
  set extended-join 1

The ability of a server to support WHOX queries is determined via a server's ISUPPORT (005) reply. If a server supports WHOX queries, Eggdrop will automatically enable this feature.

Checking Account-tracking Status
--------------------------------
While Eggdrop is running, join the partyline and type `.status`. If account-tracking is enabled (both the server supports and Eggdrop has requested), you'll see this line
::

  Loaded module information:
    #eggdroptest        : (not on channel)
    Channels: #eggdroptest (trying)
    Account tracking: Enabled           <--- This line
    Online as: BeerBot (BeerBot)

Otherwise, the prompt will tell you which required capability is missing/not enabled
::

  Loaded module information:
    #eggdroptest        :   2 members, enforcing "+tn" (greet)
    Channels: #eggdroptest (need ops)
    Account tracking: Best-effort (Missing capabilities: extended-join, see .status all for details)      <---- This line
    Online as: Eggdrop (Eggdrop)

Determining if a Server Supports Account Capabilities
-----------------------------------------------------
A server announces the capabilities it supports via a CAP request. If you have Tcl enabled on the partyline (or via a raw message from a client), you can send `.tcl cap ls` and see if the extended-join and account-notify capabilities are supported by the server. If they are not listed, the server does not support it.

A server announces if it supports WHOX via its ISUPPORT (005) announcement. If you have Tcl enabled on the partyline, you can send `.tcl issupport isset WHOX` and if it returns '1', WHOX is supported by the server.

Best-Effort Account Tracking
----------------------------
If a server only supports some, but not all, of the required capabilities, Eggdrop will switch to 'best effort' account tracking. This means Eggdrop will update account statuses whenever it sees account information, but in this mode Eggdrop cannot guarantee that all account associations are up to date.

If a server does not support extended-join, Eggdrop will not be able to determine the account associated with a user when they join. Eggdrop can update this information by sending a WHOX to the server.

If a server does not support account-notify, Eggdrop will not be able to determine the account associated with a user if they authenticate/deauthenticate from their account after joining a channel. Eggdrop can update this information by sending a WHOX to the server.

If a server does not support WHOX, Eggdrop will not be able to determine the accounts associated with users already on a channel before Eggdrop joined. There is no reliable way to update this information.

One workaround to significantly increase the accuracy of account tracking for scripts in a 'best effort' scenario would be to issue a WHOX query (assuming the server supports it), wait for the reply from the server, and then query for the account information.

account-tag
^^^^^^^^^^^
One supplementary capability that can assist a best-effort account tracking scenario is the IRCv3-defined `account-tag capability <https://ircv3.net/specs/extensions/account-tag>`_. The account-tag capability attaches a tag with the account name associated with the user sending a command. Enabling this capability allows Eggdrop to update its account tracking every time a user talks in channel, sets a mode, sends a kick, etc. While still not able to offer the same level of accuracy as enabling the "main three" account tracking features, it can increase the overall accuracy of the account list. Additionally, binds that react to user activity (pub, kick, mode, etc) containing account-tag will update the internal account list prior to executing the associated callback, so looking up the account name in the callback can be considered accurate.

Using Accounts with Tcl Scripts
-------------------------------
The Eggdrop Tcl ACCOUNT bind is triggered whenever an existing account record stored by Eggdrop is modified, such as a user de/authenticating to their account while in a channel, or information such as an account-tag being seen that updates an existing user. However, the ACCOUNT bind will NOT be triggered for the creation of a new user record, such as a user joining a channel. The bind is triggered for every channel the user is seen on- this means if a user is present with Eggdrop on four channels, the bind will be executed four times, each time with a different channel variable being passed to the associated Tcl procedure. Additionally, in a best-effort account tracking situation, Eggdrop will update the account associated with a user on all channels, not just the channel the event is seen on (and thus resulting in a bind trigger for each channel the user is on).

In order to trigger Tcl script events to cover all instances where a user logs in, you need to pair an ACCOUNT bind with a JOIN bind. This will allow you to execute account-based events when a user joins as well as if they authenticate after joining. 
