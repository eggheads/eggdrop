Last revised: October 28, 2020

.. _pbkdf2:

===============
PBKDF2 Module
===============

  Eggdrop encrypts its userfile, so users can have secure passwords.
  Eggdrop will not start without an encryption module.

  As of Eggdrop 1.9.0, a new encryption module was added, the PBKDF2 module.
  The PBKDF2 module was designed to work with the previous encryption module
  (blowfish) to make the transition easier. If this is a new bot, you can
  safely load just the PBKDF2 module. If you are transitioning a userfile from
  1.8 or earlier, you should load this AND the blowfish module. By doing so,
  Eggdrop will seamlessly update the old blowfish hashes to the new PBKDF2
  hashes once a user logs in for the first time, and allow you to (eventually)
  remove the blowfish module altogether.

  Outside of the specific case noted above, please note that if you change your
  encryption method later (i.e. using other modules like a rijndael module), you
  can't use your current userfile anymore.

  Put this line into your Eggdrop configuration file to load the server module::
    loadmodule pbkdf2

There are also some variables you can set in your config file:

  set pbkdf2-method "SHA256"
    Cryptographic hash function used. Use ``openssl list -digest-algorithms`` to view all available options

  set pbkdf2-rounds 1600
    Number of rounds. The higher the number, the longer hashing takes; but also generally the higher the protection against brute force attacks.

  This module requires: none

  Copyright (C) 2000 - 2021 Eggheads Development Team
