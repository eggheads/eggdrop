Weird Messages That Get Logged
Last revised: March 10, 2003

==============================
Weird Messages That Get Logged
==============================


    Shown below are some messages that Eggdrop might log from time to time
    that may seem a bit strange and have meanings which may not be obvious.

    (!) timer drift -- spun N minutes

      This can be caused by one of several reasons.

        - Your bot could have been swapped out of memory for a while, or for
          some reason the computer could have stopped letting the bot run. Once
          a minute, Eggdrop does a few maintenance things, including counting
          down any active Tcl timers. If for some reason, several minutes pass
          without Eggdrop being able to do this, it logs this message to let
          you know what happened. It's generally a bad thing, because it means
          that the system your bot is on is very busy, and the bot can hardly
          keep track of the channel very well when it gets swapped out for
          minutes at a time.

        - On some systems (at least Linux), if the DNS your bot is using to
          lookup hostnames is broken and *very* slow in responding (this can
          occur if the DNS server's uplink doesn't exist), then you will get
          4-5 minute timer drifts continuously. This can be fixed by loading
          the dns module.

        - The clock on your machine has just been changed. It may have been
          running behind by several minutes and was just corrected.

    (!) killmember(Nickname) -> nonexistent
      We have yet to track this down. It's a mildly bad thing, however. It
      means the bot just got informed by the server that someone left the
      channel -- but the bot has no record of that person ever being ON the
      channel.

      jwilkinson@mail.utexas.edu had some insight into this one:

        This is not an Eggdrop bug, at least not most of the time. This is a
        bug in all but perhaps the very latest ircd systems. It's not uncommon
        during netsplits and other joins for the server to lose track of killed
        or collided join notices. Also, in some servers, it is possible to
        specify non-standard characters, such as caret symbols, which get
        falsely interpreted as capital letters.

        When converted to lowercase, these symbols fail to get processed, and
        joins are not reported, although parts are.


  Copyright (C) 2003 - 2019 Eggheads Development Team
