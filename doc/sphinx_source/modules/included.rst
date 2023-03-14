Modules included with Eggdrop
=============================

.. toctree::

    mod/assoc
    mod/blowfish
    mod/channels
    mod/compress
    mod/console
    mod/ctcp
    mod/dns
    mod/filesys
    mod/ident
    mod/irc
    mod/notes
    mod/pbkdf2
    mod/seen
    mod/server
    mod/share
    mod/transfer
    mod/twitch
    mod/woobie
    mod/uptime

:ref:`assoc`
    This module provides assoc support, i.e. naming channels on the
    botnet.

:ref:`blowfish`
    Eggdrop can encrypt your userfile, so users can have secure
    passwords. Please note that when you change your encryption
    method later (i.e. using other modules like a md5 module),
    you can't use your current userfile anymore. Eggdrop will not
    start without an encryption module.

:ref:`channels`
    This module provides channel related support for the bot.
    Without it, you won't be able to make the bot join a channel
    or save channel specific userfile information.

:ref:`compress`
    This module provides support for file compression. This
    allows the bot to transfer compressed user files and, therefore,
    save a significant amount of bandwidth.

:ref:`console`
    This module provides storage of console settings when you exit
    the bot or type .store on the partyline.

:ref:`ctcp`
    This module provides the normal ctcp replies that you'd expect.
    Without it loaded, CTCP CHAT will not work.

:ref:`dns`
    This module provides asynchronous dns support. This will avoid
    long periods where the bot just hangs there, waiting for a
    hostname to resolve, which will often let it timeout on all
    other connections.

:ref:`filesys`
    This module provides an area within the bot where users can store
    and manage files. With this module, the bot is usable as a file
    server.

:ref:`irc`
    This module provides basic IRC support for your bot. You have to
    load this if you want your bot to come on IRC.

:ref:`notes`
    This module provides support for storing of notes for users from
    each other. Note sending between currently online users is
    supported in the core, this is only for storing the notes for
    later retrieval.

:ref:`seen`
    This module provides very basic seen commands via msg, on channel
    or via dcc. This module works only for users in the bot's
    userlist. If you are looking for a better and more advanced seen
    module, try the gseen module by G'Quann. You can find it at
    https://www.kreativrauschen.com/gseen.mod/ and
    https://github.com/michaelortmann/gseen.mod.

:ref:`server`
    This module provides the core server support. You have to load
    this if you want your bot to come on IRC. Not loading this is
    equivalent to the old NO_IRC define.

:ref:`share`
    This module provides userfile sharing support between two
    directly linked bots.

:ref:`transfer`
    The transfer module provides DCC SEND/GET support and userfile
    transfer support for userfile sharing.

:ref:`twitch`
    The Twitch module modifies Eggdrop to interact with the Twitch service. Twitch uses a modified implementation of IRC so not all functionality will be present. Please read doc/TWITCH for specifics on how to best use the Tiwtch module.

:ref:`uptime`
    This module reports uptime statistics to the uptime contest
    web site at https://www.eggheads.org/uptime/. Go look and see what
    your uptime is! It takes about 9 hours to show up, so if your
    bot isn't listed, try again later. See doc/settings/mod.uptime
    for more information, including details on what information is
    sent to the uptime server.

:ref:`woobie`
    This is for demonstrative purposes only. If you are looking for
    starting point in writing modules, woobie is the right thing.
