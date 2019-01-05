TLS support
Last revised: Oct 17, 2010

===========
TLS support
===========

This document provides information about TLS support which is a new
eggdrop feature since version 1.8.0.

-----
About
-----

Eggdrop can be optionally compiled with TLS support. This requires OpenSSL
0.9.8 or more recent installed on your system.
TLS support includes encryption for IRC, DCC, botnet, telnet and scripted
connections as well as certificate authentication for users and bots.

------------
Installation
------------

./configure and install as usual, the configure script will detect if your
system meets the requirements and will enable TLS automatically. You can
override the autodetection and manually disable TLS with 
./configure --disable-tls. You can't forcefully enable it though.
The configure script will look for OpenSSL at the default system locations.
If you have it installed at a non-standard location or locally in your
home directory, you'll need to specify the paths to header and library
files with the --with-sslinc and --with-ssllib options. You can also use
these if you want to override the default OpenSSL installation with a
custom one, as they take precedence over any system-wide paths.

-----
Usage
-----

By default, without additional configuration, TLS support will provide
opportunistic encryption for botnet links. For other connection types,
TLS must be requested explicitly.

Secure connections are created the same way as plaintext ones. The only
difference is that you must prefix the port number with a plus sign.
A port number that could be normally omitted, would have to be included
to enable TLS. Scripts can also switch a regular, plaintext connection
to TLS, using the starttls Tcl command.

^^^
IRC
^^^

To connect to IRC using SSL, specify the port number and prefix it with
a plus sign. Example: .jump irc.server.com +6697. The same goes for
the server list in the config file.

Some NickServ services allow you to authenticate with a certificate.
Eggdrop will use the certificte pair specified in ssl-privatekey/
ssl-certificate for authentication.

^^^^^^
Botnet
^^^^^^

Eggdrop can use TLS connections to protect botnet links if it is compiled with TLS support. TLS-enabled 1.8 bots are backwards compatible with bots that do not have TLS, whether because they are an earlier version or they were not compiled with TLS libraries. Depending on how the user configures the botnet, Eggdrop will use one of two methods to create a TLS connection: raw TLS sockets, and starttls. The difference is that a socket listening for TLS will first create a TLS connection before exchanging any eggdrop-specific data; a starttls connection will first establish the botnet link in the clear, then upgrade to a TLS connection (This means the nickname and, since v1.3.29, a challenge/response password hash are sent before TLS negotiation takes place- not the actual plaintext password).

By prefixing a listen port in the Eggdrop configuration with a plus (+), that specifies that port as a TLS-enabled port, and will only accept TLS connections (no plain text connections will be allowed). Additionally, Eggdrop 1.8 has starttls functionality, where a plain text connection can first be made to a non-TLS port (ie, one that is not prefixed with a plus) and then upgraded to a TLS connection. Currently, Eggdrop automatically attempts a starttls upgrade on all botnet connections. With two TLS-enabled Eggdrops, it graphically looks like this:

+------------------------------+----------------------------+------------------------------+
| Leaf bot sets hub port as... | and Hub bot config uses... | the connection will...       |
+------------------------------+----------------------------+------------------------------+
| port                         | listen port                | upgrade to TLS with starttls |
+------------------------------+----------------------------+------------------------------+
| port                         | listen +port               | connect with TLS             |
+------------------------------+----------------------------+------------------------------+
| +port                        | listen port                | fail. This is a known issue. |
+------------------------------+----------------------------+------------------------------+
| +port                        | listen +port               | connect with TLS             |
+------------------------------+----------------------------+------------------------------+

* Currently, adding a bot with +port and connecting to a hub listening on port does not work. This will be remedied in a future release.

To explicitly require all links to a hub be TLS-only (ie, prevent any plain text connection from being allowed), prefix the listen port in the hub configuration file with a plus (+) sign. Conversely, to force a leaf to only allow TLS (not plain text) connections with a hub, you must prefix the hub's listen port with a plus when adding it to the leaf via +bot/chaddr commands. If TLS negotiation fails and either the hub or leaf is set to require TLS, the connection is deliberately aborted and no clear text is ever sent by the TLS-requiring party.

^^^^^^^^^^
Secure DCC
^^^^^^^^^^

Eggdrop supports the SDCC protocol, allowing you to establish DCC chat
and file transfers over SSL. Example: /ctcp bot schat
Note, that currently the only IRC client supporting SDCC is KVIrc. For
information on how to initiate secure DCC chat from KVIrc (rather than
from the bot with /ctcp bot chat), consult the KVIrc documentation.

^^^^^^^
Scripts
^^^^^^^

Scripts can open or connect to TLS ports the usual way specifying the
port with a plus sign. Alternatively, the connection could be
established as plaintext and later switched on with the starttls Tcl
command. (Note that the other side should also switch to TLS at the same
time - the synchronization is the script's job, not eggdrop's.)

-------------------------------------
Keys, certificates and authentication
-------------------------------------

You need a private key and a digital certificate whenever your bot will
act as a server in a connection of any type. Common examples are hub
bots and TLS listening ports. General information about certificates and
public key infrastructure can be obtained from Internet. This document
only contains eggdrop-specific information on the subject.
The easy way to create a key and a certificate is to type 'make sslcert'
after compiling your bot (If you installed eggdrop to a non-standard
location, use make sslcert DEST=/path/to/eggdrop). This will generate a
4096-bit private key (eggdrop.key) and a certificate (eggdrop.crt) after
you fill in therequired fields.

To authenticate with a certificate instead of using password, you should
make a ssl certificate for yourself and enable ssl-cert-auth in the config
file. Then either connect to the bot using TLS and type ".fprint +" or
enter your certificate fingerprint with .fprint SHA1-FINGERPRINT.
To generate a ssl certificate for yourself, you can run the following
command from the eggdrop source directory::

  openssl req -new -x509 -nodes -keyout my.key -out my.crt -config ssl.conf

When asked about bot's handle, put your handle instead. How to use your
new certificate to connect to eggdrop, depends on your irc client.
To connect to your bot from the command line, you can use the OpenSSL
ssl client::

  openssl s_client -cert my.crt -key my.key -connect host:sslport 
    
----------------
SSL/TLS Settings
----------------
 
There are some new settings allowing control over certificate
verification and authorization.

  ssl-privatekey

    file containing Eggdrop's private key, required for the certificate.

  ssl-certificate

    Specify the filename where your SSL certificate is located.
    if your bot will accept SSL connections, it must have a certificate.

  ssl-verify-depth

    maximum verification depth when checking certificate validity.
    Determines the maximum certificate chain length to allow.

  | ssl-capath
  | ssl-cafile

    specify the location of certificate authorities certificates. These
    are used for verification. Both can be active at the same time.
    If you don't set this, validation of the issuer won't be possible and
    depending on verification settings, the peer certificate might fail
    verification.

  ssl-ciphers

    specify the list of ciphers (in order of preference) allowed for
    use with ssl.

  ssl-cert-auth

    enables or disables certificate authorization for partyline/botnet.
    This works only for SSL connections (SDCC or telnet over SSL).
    A setting of 1 means optional authorization: If the user/bot has a
    fingerprint set and it matches the certificate SHA1 fingerprint,
    access is granted, otherwise ordinary password authentication takes
    place.

    If you set this to 2 however, users without a fingerprint set or
    with a fingerprint not matching the certificate, will not be
    allowed to enter the partyline with SSL. In addition to this user and
    bot certificates will be required to have an UID field matching the
    handle of the user/bot.

  | ssl-verify-dcc
  | ssl-verify-bots
  | ssl-verify-server
  | ssl-verify-clients

    control ssl certificate verification. A value of 0 disables
    verification completely. A value of 1 enables full verification.
    Higher values enable specific exceptions like allowing self-signed
    or expired certificates. Details are documented in eggdrop.conf.
	
Copyright (C) 2010 - 2019 Eggheads Development Team
