Eggdrop, an open source IRC bot
===============================

Eggdrop is a free, open source software program built to assist in managing an IRC channel. It is the world's oldest actively-maintained IRC bot and was designed to be easily used and expanded on via it's ability to run Tcl scripts. Eggdrop can join IRC channels and perorm automated tasks such as protecting the channel from abuse, assisting users obtain their op/voice status, provide information and greetings, host games, etc.

Some things you can do with Eggdrop
-----------------------------------

Eggdrop has a large number of features, such as:

* `Channel Management <using/users.html>`_
* Running Tcl Scripts
* `Integration of the most current IRCv3 capabilities <using/ircv3.html>`_
* `The ability to link multiple Eggdrops together and share userfiles <using/botnet.html>`_
* `TLS Support <using/tls.html>`_
* `IPv6 Support <using/ipv6.html>`_
* `Twitch Support <using/twitch.html>`_
* ... and much much more!

How to get Eggdrop
------------------

The Eggdrop project source code is hosted at https://github.com/eggheads/eggdrop. You can clone it via git, or alternatively a copy of the current stable snapshot is located at https://geteggdrop.com. Additional information can be found on the official Eggdrop webpage at https://www.eggheads.org. For more information, see `Installing Eggdrop <install/install.html>`_

How to install Eggdrop
----------------------

Installation Pre-requisites
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Eggdrop requires Tcl (and its development header files) to be present on the system it is compiled on. It is also strongly encouraged to install openssl (and its development header files) to enable TLS-protected network communication.

Installation
^^^^^^^^^^^^

A guide to quickly installing Eggdrop can be found here.

Where to find more help
-----------------------

The Eggheads development team can be found lurking on #eggdrop on the Libera network (irc.libera.chat).

.. toctree::
    :caption: Installing Eggdrop
    :maxdepth: 2

    install/readme
    install/install
    install/upgrading

.. toctree::
    :caption: Using Eggdrop
    :maxdepth: 2

    using/features
    using/core
    using/partyline
    using/autoscripts
    using/users
    using/bans
    using/botnet
    using/ipv6
    using/tls
    using/ircv3
    using/accounts
    using/pbkdf2info
    using/twitchinfo
    using/tricks
    using/text-sub
    using/tcl-commands
    using/twitch-tcl-commands
    using/patch

.. toctree::
    :caption: Tutorials
    :maxdepth: 2

    tutorials/setup
    tutorials/firststeps
    tutorials/tlssetup
    tutorials/firstscript
    tutorials/module.rst

.. toctree::
    :caption: Eggdrop Modules
    :maxdepth: 2

    modules/index
    modules/included
    modules/writing
    modules/internals.rst

.. toctree::
    :caption: About Eggdrop
    :maxdepth: 2

    about/about
    about/legal
