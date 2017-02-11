Last revised: May 26, 2004

.. _filesys:

==============
Filesys Module
==============

This module provides an area within the bot where users can store and manage
files. With this module, the bot is usable as a file server.

This module requires: transfer

Put this line into your Eggdrop configuration file to load the filesys
module::

    loadmodule filesys

There are also some variables you can set in your config file:

  set files-path "/home/mydir/eggdrop/filesys"
    Set here the 'root' directory for the file system.

  set incoming-path "/home/mydir/eggdrop/filesys/incoming"
    If you want to allow uploads, set this to the directory uploads should be
    put into. Set this to "" if you don't want people to upload files to your
    bot.

  set upload-to-pwd 0
    If you don't want to have a central incoming directory, but instead
    want uploads to go to the current directory that a user is in, set this
    setting to 1.

  set filedb-path ""
    Eggdrop creates a '.filedb' file in each subdirectory of your file area
    to keep track of its own file system information. If you can't do that
    (for example, if the dcc path isn't owned by you, or you just don't want
    it to do that) specify a path here where you'd like all of the database
    files to be stored instead.

  set max-file-users 20
    Set here the maximum number of people that can be in the file area at
    once. Setting this to 0 makes it effectively infinite.

  set max-filesize 1024
    Set here the maximum allowable file size that will be received (in KB).
    Setting this to 0 makes it effectively infinite.

Copyright (C) 2000 - 2017 Eggheads Development Team
