Last revised: October 25, 2010

.. _channels:

===============
Channels Module
===============

This module provides channel related support for the bot. Without it,
you won't be able to make the bot join a channel or save channel specific
userfile information.

This module requires: none

Put this line into your Eggdrop configuration file to load the channels
module::

  loadmodule channels

There are also some variables you can set in your config file:

    set chanfile "LamestBot.chan"
      Enter here the filename where dynamic channel settings are stored.


    set force-expire 0
      Set this setting to 1 if you want your bot to expire bans/exempts/invites
      set by other opped bots on the channel.


    set share-greet 0
      Set this setting to 1 if you want your bot to share user greets with
      other bots on the channel if sharing user data.


    set use-info 1
      Set this setting to 1 if you want to allow users to store an info line.

    set allow-ps 0
      Set this setting to 1 if you want to allow both +p and +s channel modes
      to be enforced at the same time by the chanmode channel setting.
      Make sure your server supports +ps mixing or you may get endless mode
      floods.

    channel add #channel { SETTINGS }
      Add each static channel you want your bot to sit in using this command.
      There are many different possible settings you can insert into this
      command, which are explained below.

      chanmode +/-<modes>
        This setting makes the bot enforce channel modes. It will always add
        the +<modes> and remove the -<modes> modes.

      idle-kick 0
        This setting will make the bot check every minute for idle users. Set
        this to 0 to disable idle check.

      stopnethack-mode 0
        This setting will make the bot de-op anyone who enters the channel
        with serverops. There are seven different modes for this settings:

          +---+--------------------------------------------------------+
          | 0 | turn off                                               |
          +---+--------------------------------------------------------+
          | 1 | isoptest (allow serverop if registered op)             |
          +---+--------------------------------------------------------+
          | 2 | wasoptest (allow serverop if user had op before split) |
          +---+--------------------------------------------------------+
          | 3 | allow serverop if isop or wasop                        |
          +---+--------------------------------------------------------+
          | 4 | allow serverop if isop and wasop.                      |
          +---+--------------------------------------------------------+
          | 5 | If the channel is -bitch, see stopnethack-mode 3       |
          |   +--------------------------------------------------------+
          |   | If the channel is +bitch, see stopnethack-mode 1       |
          +---+--------------------------------------------------------+
          | 6 | If the channel is -bitch, see stopnethack-mode 2       |
          |   +--------------------------------------------------------+
          |   | If the channel is +bitch, see stopnethack-mode 4       |
          +---+--------------------------------------------------------+

      revenge-mode 0
        This settings defines how the bot should punish bad users when
        revenging. There are four possible settings:

          +---+--------------------------------------------------------------------------+
          | 0 | Deop the user.                                                           |
          +---+--------------------------------------------------------------------------+
          | 1 | Deop the user and give them the +d flag for the channel.                 |
          +---+--------------------------------------------------------------------------+
          | 2 | Deop the user, give them the +d flag for the channel, and kick them.     |
          +---+--------------------------------------------------------------------------+
          | 3 | Deop the user, give them the +d flag for the channel, kick, and ban them.|
          +---+--------------------------------------------------------------------------+

      ban-type 3
	This setting defines what type of bans should eggdrop place for
	+k users or when revenge-mode is 3. Available types are:

          +---+------------------------+
          | 0 \*!user\@host            |
          +---+------------------------+
          | 1 \*!\*user\@host          |
          +---+------------------------+
          | 2 \*!\*\@host              |
          +---+------------------------+
          | 3 \*!\*user\@\*.host       |
          +---+------------------------+
          | 4 \*!\*\@*.host            |
          +---+------------------------+
          | 5 nick!user\@host          |
          +---+------------------------+
          | 6 nick!\*user\@host        |
          +---+------------------------+
          | 7 nick!\*\@host            |
          +---+------------------------+
          | 8 nick!\*user\@*.host      |
          +---+------------------------+
          | 9 nick!\*\@*.host          |
          +---+------------------------+

	You can also specify types from 10 to 19 which correspond to types
	0 to 9, but instead of using a * wildcard to replace portions of the
	host, only numbers in hostnames are replaced with the '?' wildcard.
	Same is valid for types 20-29, but instead of '?', the '*' wildcard
	will be used. Types 30-39 set the host to '*'.

      ban-time 120
        Set here how long temporary bans will last (in minutes). If you
        set this setting to 0, the bot will never remove them.

      exempt-time 60
        Set here how long temporary exempts will last (in minutes). If you set
        this setting to 0, the bot will never remove them. The bot will check
        the exempts every X minutes, but will not remove the exempt if a ban is
        set on the channel that matches that exempt. Once the ban is removed,
        then the exempt will be removed the next time the bot checks. Please
        note that this is an IRCnet feature.

      invite-time 60
        Set here how long temporary invites will last (in minutes). If you set
        this setting to 0, the bot will never remove them. The bot will check
        the invites every X minutes, but will not remove the invite if a
        channel is set to +i. Once the channel is -i then the invite will be
        removed the next time the bot checks. Please note that this is an
        IRCnet feature.

      aop-delay (minimum:maximum)
        This is used for autoop, autohalfop, autovoice. If an op or voice joins
        a channel while another op or voice is pending, the bot will attempt to
        put both modes on one line.

          +--------------+-----------------------------------------+
          | aop-delay 0  | No delay is used.                       |
          +--------------+-----------------------------------------+
          | aop-delay X  | An X second delay is used.              |
          +--------------+-----------------------------------------+
          | aop-delay X:Y| A random delay between X and Y is used. |
          +--------------+-----------------------------------------+

       need-op { putserv "PRIVMSG #lamest :op me cos i'm lame!" }
         This setting will make the bot run the script enclosed in braces
         if it does not have ops. This must be shorter than 120 characters.
         If you use scripts like getops.tcl or botnetop.tcl, you don't need
         to set this setting.

       need-invite { putserv "PRIVMSG #lamest :let me in!" }
         This setting will make the bot run the script enclosed in braces
         if it needs an invite to the channel. This must be shorter than 120
         characters. If you use scripts like getops.tcl or botnetop.tcl, you
         don't need to set this setting.

       need-key { putserv "PRIVMSG #lamest :let me in!" }
         This setting will make the bot run the script enclosed in braces
         if it needs the key to the channel. This must be shorter than 120
         characters. If you use scripts like getops.tcl or botnetop.tcl, you
         don't need to set this setting.

       need-unban { putserv "PRIVMSG #lamest :let me in!" }
         This setting will make the bot run the script enclosed in braces
         if it needs to be unbanned on the channel. This must be shorter than
         120 characters. If you use scripts like getops.tcl or botnetop.tcl,
         you don't need to set this setting.

       need-limit { putserv "PRIVMSG #lamest :let me in!" }
         This setting will make the bot run the script enclosed in braces
         if it needs the limit to be raised on the channel. This must be
         shorter than 120 characters. If you use scripts like getops.tcl or
         botnetop.tcl, you don't need to set this setting.

       flood-chan 15:60
         Set here how many channel messages in how many seconds from one
         host constitutes a flood. Setting this to 0, 0:X or X:0 disables text
         flood protection for the channel, where X is an integer >= 0.

       flood-deop 3:10
         Set here how many deops in how many seconds from one host constitutes
         a flood. Setting this to 0, 0:X or X:0 disables deop flood protection for
         the channel, where X is an integer >= 0.

       flood-kick 3:10
         Set here how many kicks in how many seconds from one host constitutes
         a flood. Setting this to 0, 0:X or X:0 disables kick flood protection for
         the channel, where X is an integer >= 0.

       flood-join 5:60
         Set here how many joins in how many seconds from one host constitutes
         a flood. Setting this to 0, 0:X or X:0 disables join flood protection for
         the channel, where X is an integer >= 0.

       flood-ctcp 3:60
         Set here how many channel ctcps in how many seconds from one host
         constitutes a flood. Setting this to 0, 0:X or X:0 disables ctcp flood
         protection for the channel, where X is an integer >= 0.

       flood-nick 5:60
         Set here how many nick changes in how many seconds from one host
         constitutes a flood. Setting this to 0, 0:X or X:0 disables nick flood
         protection for the channel, where X is an integer >= 0.


    channel set <chan> +/-<setting>
      There are many different options for channels which you can define.
      They can be enabled or disabled by a plus or minus in front of them.

      A complete list of all available channel settings:

        enforcebans
          When a ban is set, kick people who are on the channel and match
          the ban?

        dynamicbans
          Only activate bans on the channel when necessary? This keeps the
          channel's ban list from getting excessively long. The bot still
          remembers every ban, but it only activates a ban on the channel
          when it sees someone join who matches that ban.

        userbans
          Allow bans to be made by users directly? If turned off, the bot will
          require all bans to be made through the bot's console.

        dynamicexempts
          Only activate exempts on the channel when necessary? This keeps the
          channel's exempt list from getting excessively long. The bot still
          remembers every exempt, but it only activates a exempt on the channel
          when it sees a ban set that matches the exempt. The exempt remains
          active on the channel for as long as the ban is still active.

        userexempts
          Allow exempts to be made by users directly? If turned off, the bot will
          require all exempts to be made through the bot's console.

        dynamicinvites
          Only activate invites on the channel when necessary? This keeps the
          channel's invite list from getting excessively long. The bot still
          remembers every invite, but the invites are only activated when the
          channel is set to invite only and a user joins after requesting an
          invite. Once set, the invite remains until the channel goes to -i.

        userinvites
          Allow invites to be made by users directly? If turned off, the bot
          will require all invites to be made through the bot's console.

        autoop
          Op users with the +o flag as soon as they join the channel?
          This is insecure and not recommended.

        autohalfop
          Halfop users with the +l flag as soon as they join the channel?
          This is insecure and not recommended.

        bitch
          Only let users with the +o flag have op on the channel?

        greet
          Say a user's info line when they join the channel?

        protectops
          Re-op a user with the +o flag if they get deopped?

        protecthalfops
          Re-halfop a user with the +l flag if they get dehalfopped?

        protectfriends
          Re-op a user with the +f flag if they get deopped?

        statuslog
          Log the channel status line every 5 minutes? This shows the bot's
          status on the channel (op, voice, etc.), the channel's modes, and
          the total number of members, ops, voices, regular users, and +b,
          +e, and +I modes on the channel. A sample status line follows:

            [01:40] @#lamest (+istn) : [m/1 o/1 v/4 n/7 b/1 e/5 I/7]

        revenge
          Remember people who deop/kick/ban the bot, valid ops, or friends
          and punish them? Users with the +f flag are exempt from revenge.

        revengebot
          This is similar to to the 'revenge' option, but it only triggers
          if a bot gets deopped, kicked or banned.

        autovoice
          Voice users with the +v flag when they join the channel?

        secret
          Prevent this channel from being listed on the botnet?

        shared
          Share channel-related user info for this channel?

        cycle
          Cycle the channel when it has no ops?

        dontkickops
          Do you want the bot not to be able to kick users who have the +o
          flag, letting them kick-flood for instance to protect the channel
          against clone attacks?

        inactive
          This prevents the bot from joining the channel (or makes it leave the
          channel if it is already there). It can be useful to make the bot leave
          a channel without losing its settings, channel-specific user flags,
          channel bans, and without affecting sharing.

        seen
          Respond to seen requests in the channel?  The seen module must be
          loaded for this to work.

        nodesynch
          Allow non-ops to perform channel modes? This can stop the bot from
          fighting with services such as ChanServ, or from kicking IRCops when
          setting channel modes without having ops.

        static
          Allow only permanent owners to remove the channel?

    The following settings are used as default values when you .+chan #chan or .tcl
    channel add #chan. Look in the section above for explanation of every option.

      set default-flood-chan 15:60

      set default-flood-deop 3:10

      set default-flood-kick 3:10

      set default-flood-join 5:60

      set default-flood-ctcp 3:60

      set default-flood-nick 5:60

      set default-aop-delay 5:30

      set default-idle-kick 0

      set default-chanmode "nt"

      set default-stopnethack-mode 0

      set default-revenge-mode 0

      set default-ban-type 3

      set default-ban-time 120

      set default-exempt-time 60

      set default-invite-time 60

      set default-chanset {
        | -autoop         
        | -autovoice
        | -bitch          
        | +cycle
        | +dontkickops    
        | +dynamicbans
        | +dynamicexempts
        | +dynamicinvites
        | -enforcebans   
        | +greet
        | -inactive      
        | -nodesynch
        | -protectfriends
        | +protectops
        | -revenge       
        | -revengebot
        | -secret       
        | -seen
        | +shared        
        | -statuslog
        | +userbans     
        | +userexempts
        | +userinvites   
        | -protecthalfops
        | -autohalfop    
        | -static

      }

  Copyright (C) 2000 - 2019 Eggheads Development Team
