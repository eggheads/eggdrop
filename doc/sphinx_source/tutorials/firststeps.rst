Common First Steps
==================

Log on to the partyline
-----------------------
Now that your bot is online, you'll want to join the partyline to further use the bot. First, read what it tells you when you started it up::

  STARTING BOT IN USERFILE CREATION MODE.
  Telnet to the bot and enter 'NEW' as your nickname.
  OR go to IRC and type:  /msg BotNick hello
  This will make the bot recognize you as the master.

You can either telnet to the bot, or connect to the bot using DCC Chat. To telnet, you'll either need a program like Putty (Windows), or you can do it from the command line of your shell using the telnet command::

  telnet <IP of bot> <listen port>

You can find the IP and port the bot is listening on by a) remembering what you set in the config file ;) or b) reading the display the bot presented when it started up. Look for a line that looks similar to this::

  Listening for telnet connections on 2.4.6.9:3183 (all).

This tells you that the bot is listening on IP 2.4.6.9, port 3183. If you see 0.0.0.0 listed, that means Eggdrop is listening on all available IPs on that particular host.


If you choose not to telnet to connect to the partyline, you can either ``/dcc chat BotNick`` or ``/ctcp BotNick chat``. If one of those methods does not work for you, try the other. Once you're on the bot for the first time, type ``.help`` for a short list of available commands, or ``.help all`` for a more thorough list.

Common first steps
------------------

To learn more about any of these commands, type .help <command> on the partyline. It will provide you the syntax you need, as well as a short description of how to use the command.

Join a Channel
^^^^^^^^^^^^^^

To tell the Eggdrop to join a channel, use::

  .+chan #channel

Add a User
^^^^^^^^^^

To add a user to the bot, use::

  .+user <handle> 

Add a Host to a User
^^^^^^^^^^^^^^^^^^^^

The handle is the name that the bot uses to track a user. No matter what nickname on IRC a user uses, a single handle is used to track the user by their hostmask. To add a hostmask of a user to a handle, use::

  .+host <handle> <hostmask>

where the hostmask is in the format of <nick>!<ident>@hostname.com . Wildcards can be used; common formats are \*!\*@hostname.com for static hosts, or \*!ident@*.foo.com for dynamic hostnames.

Assign Permission Flags
^^^^^^^^^^^^^^^^^^^^^^^

To assign an access level to a user, first read ``.help whois`` for a listing of possible access levels and their corresponding flags. Then, assign the desired flag to the user with::

  .chattr <+flag> <handle>

So to grant a user the voice flag, you would do::

  .chattr +v handle

It is important to note that, when on the partyline, you want to use the handle of the user, not their current nickname.

Configure Channel Settings
^^^^^^^^^^^^^^^^^^^^^^^^^^

Finally, Eggdrop is often used to moderate and control channels. This is done via the ``.chanset`` command. To learn more about the (numerous!) settings that can be used to control a channel, read::

  .help chaninfo

Common uses involve setting channels modes. This can be done with the chanmode channel setting::

  .chanset #channel chanmode +snt

which will enforce the s, n, and t flags on a channel.

Automatically restarting an Eggdrop
-----------------------------------

A common question asked by users is, how can I configure Eggdrop to automatically restart should it die, such as after a reboot? Historically, Eggdrop relied on the host's crontab system to run a script (called botchk) every ten minutes to see if the eggdrop is running. If the eggdrop is not running, the script will restart the bot, with an optional email sent to the user informing them of the action. Newer Linux systems come with systemd, which can provide better real-time monitoring of processes such as Eggdrop. You probably want to use systemd if your system has it.

Crontab Method
^^^^^^^^^^^^^^

1. Enter the directory you installed your Eggdrop to. Most commonly, this is ~/eggdrop (also known as /home/<username>/eggdrop).

2. Just humor us- run ``./scripts/autobotchk`` without any arguments and read the options available to you. They're listed there for a reason! 

3. If you don't want to customize anything via the options listed in #2, you can install a crontab job to start Eggdrop simply by running::

    ./scripts/autobotchk yourEggdropConfigNameHere.conf 

4. Review the output of the script, and verify your new crontab entry by typing::

    crontab -l

By default, it should create an entry that looks similar to::

    0,10,20,30,40,50 * * * * /home/user/bot/scripts/YourEggdrop.botchk 2>&1

This will run the generated botchk script every ten minutes and restart your Eggdrop if it is not running during the check. Also note that if you run autobotchk from the scripts directory, you'll have to manually specify your config file location with the -dir option. To remove a crontab entry, use ``crontab -e`` to open the crontab file in your system's default editor and remove the crontab line.

Systemd Method (Newer Linux Systems)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Enter the directory you installed your Eggdrop to. Most commonly, this is ~/eggdrop (also known as /home/<username>/eggdrop).

2. Install the systemd job for Eggdrop simply by running::

    ./scripts/autobotchk yourEggdropConfigNameHere.conf -systemd

3. Note the output at the end of the script informing you of the command you can use to start/stop the Eggdrop in thee future. For example, to manually start the Eggdrop, use::

    systemctl --user start <botname>.service

To stop Eggdrop, use::

    systemctl --user stop <botname>.service

To rehash (not reload) Eggdrop, use::

    systemctl --user reload <botname>.service

(Yes, we acknowledge the confusion that the systemd reload command will execute the Eggdrop '.rehash' command, not the '.reload' command. Unfortunately, systemd did not consult us when choosing its commands!)

To prevent Eggdrop from automatically running after a system start, use::

    systemctl --user disable <botname>.service 

To re-enable Eggdrop automatically starting after a system start, use::

    systemctl --user enable <botname>.service

Authenticating with NickServ
----------------------------

Many IRC features require you to authenticate with NickServ to use them. You can do this from your config file by searching for the line::

    #  putserv "PRIVMSG NickServ :identify <password>"

in your config file. Uncomment it by removing the '#' sign and then replace <password> with your password. Your bot will now authenticate with NickServ each time it joins a server.

Setting up SASL authentication
------------------------------

Simple Authentication and Security Layer (SASL) is becoming a prevalant method of authenticating with IRC services such as NickServ prior to your client finalizing a connection to the IRC server, eliminating the need to /msg NickServ to identify yourself. In other words, you can authenticate with NickServ and do things like receive a cloaked hostmask before your client ever appears on the IRC server. Eggdrop supports three methods of SASL authentication, set via the sasl-mechanism setting:

* **PLAIN**: To use this method, set sasl-mechanism to 0. This method passes the username and password (set in the sasl-username and sasl-password config file settings) to the IRC server in plaintext. If you only connect to the IRC server using a connection protected by SSL/TLS this is a generally safe method of authentication; however you probably want to avoid this method if you connect to a server on a non-protected port as the exchange itself is not encrypted.

* **ECDSA-NIST256P-CHALLENGE**: To use this method, set sasl-mechanism to 1. This method uses a public/private keypair to authenticate, so no username/password is required. Not all servers support this method. If your server does support this, you you must generate a certificate pair using::

    openssl ecparam -genkey -name prime256v1 -out eggdrop-ecdsa.pem

  You will need to determine your public key fingerprint by using::

    openssl ec -noout -text -conv_form compressed -in eggdrop-ecdsa.pem | grep '^pub:' -A 3 | tail -n 3 | tr -d ' \n:' | xxd -r -p | base64

  Then, authenticate with your NickServ service and register your public certificate with NickServ. You can view your public key  On Libera for example, it is done by::

    /msg NickServ set pubkey <fingerprint string from above goes here>

* **EXTERNAL**: To use this method, set sasl-mechanism to 2. This method allows you to use other TLS certificates to connect to the IRC server, if the IRC server supports it. An EXTERNAL authentication method usually requires you to connect to the IRC server using SSL/TLS. There are many ways to generate certificates; one such way is generating your own certificate using::

    openssl req -new -x509 -nodes -keyout eggdrop.key -out eggdrop.crt

You will need to determine your public key fingerprint by using::

    openssl x509 -in eggdrop.crt -outform der | sha1sum -b | cut -d' ' -f1

Then, ensure you have those keys loaded in the ssl-privatekey and ssl-certificate settings in the config file. Finally, to add this certificate to your NickServ account, type::

    /msg NickServ cert add <fingerprint string from above goes here>

* **SCRAM-SHA-256**: To use this method, set sasl-mechanism to 3.

* **SCRAM-SHA-512**: To use this method, set sasl-mechanism to 4.
