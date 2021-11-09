IRCv3 support
Last revised: Jun 20, 2021

=============
IRCv3 support
=============


This document provides information about IRCv3 capabilities, as defined via specifications documented by the IRCv3 working group (`<https://ircv3.net/>`_). Support for some of these specifications was added starting with version 1.9.0, and more capabilites are added as possible with new versions.

-----
About
-----

As more and more IRC servers began to develop and implement their own versions of the IRC protocol (generally defined in RFC1459 and RFC2812), a working group comprised of server, client, and bot developers decided to work together to document these features to make their implementation defined and standardized across servers. What emerged was the IRCv3 set of standards. The specifications developed by the IRCv3 working group was designed to be backwards compatible and are generally implemented via a CAP (capability) request sent at the initialization of an IRC session. A client can optinoally request these "extra" capabilities be enabled through the CAP request, with the assumption that the client can then support the capability requested and enabled. Not all servers or clients support the same capabilites, a general support list can be via the appropriate support table link at `<https://ircv3.net/>`_. 

-----
Usage
-----

Within eggdrop.conf, several common IRCv3-defined capabilities are enabled simply changing their setting to '1'. Other capabilities without explicit settings in eggdrop.conf may be requested by adding them in a space-separated list to the cap-request setting. For more information on what a specific IRCv3-defined capability does, please consult `<https://ircv3.net/irc/>`_.

--------------------------
Supported CAP capabilities
--------------------------

The following capabilites are supported by Eggdrop:

 * CAP requests
 * SASL 3.1
 * account-notify
 * account-tag
 * away-notify
 * chghost
 * echo-message
 * extended-join
 * invite-notify
 * message-tags
 * server-time
 * setname
 * +typing

Copyright (C) 2010 - 2021 Eggheads Development Team
