Last revised: June 20, 2026

.. _compress:

===============
Compress Module
===============

This module provides support for file compression. It can be used
to compress files via Tcl or to transfer the userfile compressed during the
share process, saving bandwidth.

This module requires: share

Put this line into your Eggdrop configuration file to load the compress
module::

  loadmodule compress

There are also some variables you can set in your config file:

  set share-compressed 1
    Allow compressed sending of user files? The user files are compressed
    with the compression level defined in 'compress-level'.

  set compress-level 9
    This is the default compression level used. These levels are the same
    as those used by GNU gzip.

  set max-uncompress-size 16777216
    Maximum size for output file for uncompress function in bytes.


Copyright (C) 2000 - 2025 Eggheads Development Team
