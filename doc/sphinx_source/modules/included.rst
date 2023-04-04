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
    This module has been deprecated in favor of the pbkdf2 module for hashing purposes, such as passwords in the userfile. However, it is still required for encrypting/decrypting strings.

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

:ref:`ident`
    This module adds Eggdrop support for the externally-provided oident service, or alternatively the ability for Eggdrop to act as its own ident daemon.

:ref:`irc`
    This module provides basic IRC support for your bot. You have to
    load this if you want your bot to come on IRC.

:ref:`notes`
    This module provides support for storing of notes for users from
    each other. Note sending between currently online users is
    supported in the core, this is only for storing the notes for
    later retrieval.

:ref:`pbkdf2`
    This modules updates Eggdrop to use PBKDF2 for hashing purposes, such as for userfile passwords. It was specifically designed to work with the blowfish module to make the transition from blowfish to pbkdf2 password hashing as easy as possible. If you are transitioning a userfile from 1.8 or earlier, you should load this AND the blowfish module. By doing so, Eggdrop will seamlessly update the old blowfish hashes to the new PBKDF2 hashes once a user logs in for the first time, and allow you to (eventually) remove the blowfish module altogether. For new bots, you should load this module by itself and not use the blowfish module. The blowfish module is still required for encrypting/decrypting strings. Eggdrop will not start without an encryption module loaded.

:ref:`seen`
    This module provides very basic seen commands via msg, on channel
    or via dcc. This module works only for users in the bot's
    userlist. If you are looking for a better and more advanced seen
    module, try the gseen module by G'Quann. You can find it at
    http://www.kreativrauschen.com/gseen.mod/.

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
    web site at http://uptime.eggheads.org. Go look and see what
    your uptime is! It takes about 9 hours to show up, so if your
    bot isn't listed, try again later. See doc/settings/mod.uptime
    for more information, including details on what information is
    sent to the uptime server.

:ref:`woobie`
    This is for demonstrative purposes only. If you are looking for
    starting point in writing modules, woobie is the right thing.
