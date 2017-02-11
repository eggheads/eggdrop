Last revised: January 1, 2002

.. _transfer:

===============
Transfer Module
===============

The transfer module provides DCC SEND/GET support and userfile transfer
support for userfile sharing.

This module requires: none

Put this line into your Eggdrop configuration file to load the transfer
module::

  loadmodule transfer

There are also some variables you can set in your config file:

  set max-dloads 3
    Set here the maximum number of simultaneous downloads to allow for
    each user.

  set dcc-block 0
    Set here the block size for dcc transfers. ircII uses 512 bytes,
    but admits that may be too small. 1024 is standard these days.
    Set this to 0 to use turbo-dcc (recommended).

  set copy-to-tmp 1
    Enable this setting if you want to copy files into the /tmp directory
    before sending them. This is useful on most systems for file stability,
    but if your directories are NFS mounted, it's a pain, and you'll want
    to set this to 0. If you are low on disk space, you may also want to
    set this to 0.

  set xfer-timeout 30
    Set here the time (in seconds) to wait before an inactive transfer
    times out.

Copyright (C) 2000 - 2017 Eggheads Development Team
