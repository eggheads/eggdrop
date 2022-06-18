*********
Upgrading
*********
Last revised: November 27, 2021

########################################
Upgrading Eggdrop from v1.6/v1.8 -> v1.9
########################################

  What's new? To gain a full understanding of changes to the Eggdrop
  v1.9 version line, you can read the following documents:

    INSTALL
    README
    doc/TLS
    doc/IPV6
    doc/Changes1.9
    doc/tcl-commands.doc

  All of these documents combined will fill you in on the latest changes to
  Eggdrop in version 1.9.x. All files, with the exception of Changes1.9, are
  also available in html format in doc/html/.

  For support, feel free to visit us on Libera #eggdrop.

  If you are upgrading from a pre-1.6 version of Eggdrop:

    1. Before you start the bot for the first time, BACKUP your userfile.

    2. DON'T USE YOUR OLD CONFIG FILE. MAKE A NEW ONE!

######################################################
Must-read changes made to Eggdrop v1.9 from Eggdrop1.8
######################################################

These are NOT all changes or new settings; rather just the "killer" changes that may directly affect Eggdrop's previous performance without modification.

Config
******

To migrate from a 1.8 to a 1.9 Eggdrop, some changes are suggested to be made in your configuration file:

* Eggdrop has deprecated the blowfish module for password hashing in favor of the PBKDF2 module. This is a BIG change which, if done carelessly, has the potential to render stored passwords useless. Please see doc/PBKDF2 for information on how to properly migrate your userfiles and passwords to the new module.
* Eggdrop 1.9 switched from the "set servers {}" syntax to the "server add" command. For example, if your configuration file previously had::

    set servers {
      my.server.com:6667
    }

  you should now instead use::

    server add my.server.com 6667

  Please read the config file for additional examples

* Eggdrop no longer requires the '-n' flag to start Eggdrop in terminal mode.


Modules
*******

While most 3rd party modules that worked on Eggdrop v1.6/v1.8 should still work with Eggdrop v1.9, many of them contain a version check that only allows them to run on 1.6.x bots. We have removed the version check from some of the more popular modules provide them at `<ftp://eggheads.org/pub/eggdrop/modules/1.9/>`_

Scripts
*******

All 3rd party Tcl scripts that work with Eggdrop v1.6/v1.8 should fully work with Eggdrop v1.9.

Botnet
******

In Eggdrop v1.8, Eggdrop bots would automatically attempt to upgrade any botnet link to an SSL/TLS connection. In v1.9, the user is required to explicitly request an SSL/TLS connection by prefixing the port with a '+'. If you wish your botnet to take advantage of encryption, use the .chaddr command to update your ports to start with a '+'.

Tcl Commands
************

A lot of backwards-compatible additions and changes have been made to Tcl commands. Please look at doc/tcl-commands.doc to see them.

Documentation
*************

Documentation has been updated to reflect new and removed commands and variables. Almost all files have changed, so take a look at them.
