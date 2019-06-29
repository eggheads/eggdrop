Last revised: March 07, 2002

===============
Users and Flags
===============

People on IRC are recognized by the bot according to their
nick!user@host. That is, if I am on IRC as:

   \*\*\* Robey is robey\@hubcap.clemson.edu (i hate milk)

Eggdrop will identify me according to "Robey!robey\@hubcap.clemson.edu"
and not only by my nickname.

Eggdrop does not have access levels like some bots. There are no
meaningless numbers or titles. Instead, each user has "flags" that
entitle them to certain privileges. Think of a flag as a badge. Any user
can have any number of flags -- you can have no flags, or you can have
all of them. Some flags are good, some are bad. Each flag is identified
by a letter. A channel flag applies only to a specific channel, and a
global flag applies to all channels. The standard global flags are:

  +---+-----------------+-------------------------------------------------------+
  | n | owner           | user has absolute control. Only give this flag to     |
  |   |                 | people you trust completely.                          |
  +---+-----------------+-------------------------------------------------------+
  | m | master          | user has access to almost every feature of the bot.   |
  +---+-----------------+-------------------------------------------------------+
  | t | botnet-master   | user has access to all features dealing with the      |
  |   |                 | botnet.                                               |
  +---+-----------------+-------------------------------------------------------+
  | a | auto-op         | user is opped automatically upon joining a channel.   |
  +---+-----------------+-------------------------------------------------------+
  | o | op              | user has op access to all of the bot's channels.      |
  +---+-----------------+-------------------------------------------------------+
  | y | auto-halfop     | user is halfopped automatically upon joining a channel|
  +---+-----------------+-------------------------------------------------------+
  | l | halfop          | user has halfop access to all of the bot's channels.  |
  +---+-----------------+-------------------------------------------------------+
  | g | auto-voice      | user is voiced automatically upon joining a channel.  |
  +---+-----------------+-------------------------------------------------------+
  | v | voice           | user gets +v automatically on +autovoice channels.    |
  +---+-----------------+-------------------------------------------------------+
  | f | friend          | user is not punished for flooding, etc.               |
  +---+-----------------+-------------------------------------------------------+
  | p | party           | user has access to the partyline.                     |
  +---+-----------------+-------------------------------------------------------+
  | q | quiet           | user does not get voice on +autovoice channels.       |
  +---+-----------------+-------------------------------------------------------+
  | r | dehalfop        | user cannot gain halfops on any of the bot's channels.|
  +---+-----------------+-------------------------------------------------------+
  | d | deop            | user cannot gain ops on any of the bot's channels.    |
  +---+-----------------+-------------------------------------------------------+
  | k | auto-kick       | user is kicked and banned automatically.              |
  +---+-----------------+-------------------------------------------------------+
  | x | xfer            | user has access to the file transfer area of the bot  |
  |   |                 | the bot.                                              |
  +---+-----------------+-------------------------------------------------------+
  | j | janitor         | user can perform maintenance in the file area of the  |
  |   |                 | bot (if it exists) -- like a "master" of the file     |
  |   |                 | area. Janitors have complete access to the filesystem.|
  +---+-----------------+-------------------------------------------------------+
  | c | common          | this marks a user who is connecting from a public site|
  |   |                 | from which any number of people can use IRC. The user |
  |   |                 | will now be recognized by NICKNAME.                   |
  +---+-----------------+-------------------------------------------------------+
  | b | bot             | user is a bot.                                        |
  +---+-----------------+-------------------------------------------------------+
  | w | wasop-test      | user needs wasop test for +stopnethack procedure.     |
  +---+-----------------+-------------------------------------------------------+
  | z | washalfop-test  | user needs washalfop test for +stopnethack procedure. |
  +---+-----------------+-------------------------------------------------------+
  | e | nethack-exempt  | user is exempted from stopnethack protection.         |
  +---+-----------------+-------------------------------------------------------+
  | u | unshared        | user record is not sent to other bots.                |
  +---+-----------------+-------------------------------------------------------+
  | h | highlight       | use bold text in help/text files.                     |
  +---+-----------------+-------------------------------------------------------+

  All global flags other then u, h, b, c, x, j, and p are also
  channel-specific flags. Flags are set with the chattr command.
  The syntax for this command is::

    chattr <nickname> [attributes] [channel]

  There are also 26 global user-defined flags and 26 channel user-defined
  flags. These are used by scripts, and their uses very depending on the
  script that uses them.

Copyright (C) 2002 - 2019 Eggheads Development Team
