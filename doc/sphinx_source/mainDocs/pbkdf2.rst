Last revised: October 28, 2020

**************
PBKDF2 Hashing
**************

    With the release of Eggdrop 1.9.0, the development team is releasing an updated crytopgraphy module, PBKDF2, for use with Eggdrop. PBKDF2 is a one-way hashing algorithm used to protect the contents of the password file, as well as for use via the Tcl interface.

Background
==========
    Prior to Eggdrop 1.9.0, the blowfish module was included with Eggdrop to protect sensitive data such as passwords stored in the userfile. While there are no known practical attacks against blowfish at the time of this writing, it was decided that a more modern crypto solution was desirable to be included with Eggdrop. The PBKDF2 (Password-based Key Derivation Function 2) uses a password and salt value to create a password hash (the salt value ensures that the hashes of two identical passwords are different). This process is one-way, which means the hashes cannot be cryptographically reversed and thus are safe for storing in a file.

    The default configuration of Eggdrop 1.9.0 has both the blowfish and pbkdf2 modules enabled (see `Hybrid Configuration`_ below). This will allow users upgrading a seamless transition to the PBKDF2 module. For users starting an Eggdrop for the first time, it is recommended to comment out the 'loadmodule blowfish' line, in order to implement the `Solo Configuration`_.

    Also of note, the maximum password length is increased to 30 with PBKDF2, up from 15 with Blowfish. The automatically-generated botnet passwords are now 30 characters instead of the maximum-allowed 16 used with the blowfish module, and pull from a larger character set than what was used with the blowfish module. Finally, if you are linking bots, you'll need to ensure the same module is loaded on both bots (ie, if the hub bot is using the pbkdf2 module, the leaf bots must have pbkdf2 loaded as well in order to enable authentication checks).

Usage
=====

    There are two ways to implement PBKDF2- Hybrid configuration, which is recommended for transitioning an already-existing userfile to PBKDF2 by working with the blowfish module, and Solo configuration, which is recommended for use when starting a new Eggdrop for the first time.

Hybrid Configuration
--------------------

    With a hybrid configuration, Eggdrop will run both the blowfish and the pbkdf2 modules concurrently. This will allow Eggdrop to authenticate users against their existing blowfish passwords stored in the userfile. However, the first time a user logs in, the pbkdf2 module will hash the (correct) password they enter and save it to the userfile. The pbkdf2-hashed password will then be used for all future logins.

Enabling hybrid configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#. BACK UP YOUR USERFILE! This is the file that usually ends with '.user'. 

#. Ensure

::

    loadmodule blowfish

is added to the config file and not commented out (it should already be there).

#. Ensure

::
    loadmodule pbkdf2

is uncommented in the config file (or added, if this is a config file from 1.8)


#. Start Eggdrop

#. If this is your first time starting this Eggdrop, follow the instructions it gives you at startup to identify yourself. Otherwise, for an existing Eggdrop where you are already added to the userfile, log in as usual to the partyline via telnet or DCC, or authenticate via a message command like /msg bot op <password>

#. Sit back and enjoy a cold beverage, you did it! Now encourage the rest of your users to log in so that their passwords are updated to the new format as well.

Solo configuration
------------------

    With a solo configuration, Eggdrop will only run the pbkdf2 module. Eggdrop will not be able to authenticate against passwords in an already-existing userfile and thus will require every user to set a password again, as if they were just added to Eggdrop. This can be done via the PASS msg command (/msg bot PASS <password>) or by having a user with appropriate permissions (and an already-set password) log into the partyline and use the '.chpass' command.

    SECURITY CONSIDERATION: This configuration is not ideal for transitioning an existing userfile to PBKDF2. Without the blowfish module loaded, every user in the userfile essentially has no password set. This means any other user that matches a hostmask applied to a handle (*!*@*.aol.com, I'm looking at you) could set the password and gain access to that user's Eggdrop account.

Enabling solo configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

#. BACK UP YOUR USERFILE! This is the file that usually ends with '.user'.

#. Remove or comment::

    # loadmodule blowfish

from your config file. 

#. Ensure::

    loadmodule pbkdf2

is uncommented (or added, if this is a config file from 1.8) from your config file.

#. Start Eggdrop

#. If this is your first time starting this Eggdrop, follow the instructions it gives you at startup to identify yourself. Otherwise, for an existing Eggdrop where you are already added to the userfile, set a new password via /msg bot PASS <password>

#. Sit back and enjoy a fancy lobster dinner, you did it! If there are other users already added to the bot, DEFINITELY encourage them to set a new password IMMEDIATELY!

Tcl Interface
=============

The PBKDF2 module adds the 'encpass2' command to the Tcl library. This command takes a string and hashes it using the PBKDF2 algorithm, and returns a string in the following format::

    $<PBK method>$rounds=<rounds>$<salt>$<password hash>

where 'PBK method' is the method specified in the configuration file, 'rounds' is the number of rounds specified in the configuration file, 'salt' is the value used for the salt, and 'password hash' is the output of the hashing algorithm.


Copyright (C) 2000 - 2021 Eggheads Development Team
