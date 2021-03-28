IPv6 support
Last revised: Mar 24, 2021

============
IPv6 support
============


This document provides information about IPv6 support which is a new
eggdrop feature since version 1.8.0.

-----
About
-----

Eggdrop can be compiled with IPv6 support. To make use of this, you need an
IPv6-enabled OS and IPv6 connectivity.
Every possible type of TCP connection can be established over IPv6 now,
which includes IRC connections, DCC connections, file transfer, botnet
connections, Tcl script connections initiated with the listen/connect
commands, telnet and ident lookups.

------------
Installation
------------

./configure and install as usual, the configure script will detect if your
system supports IPv6 and will enable it automatically. You can override this
behavior and manually enable or disable IPv6 with ./configure --enable-ipv6
or ./configure --disable-ipv6.
Older operating systems may have limited or no support for IPv6. Linux 2.4 &
2.6, FreeBSD, NetBSD, OpenBSD and Mac OS X all have full IPv6 support.
MS Windows has proper support beginning with Windows Vista. XP's IPv6 stack
has some limitations and needs to be manually installed and enabled. Cygwin
includes IPv6 only since version 1.7. Unofficial patches are available for
1.5.x.

-----
Usage
-----

You can use IPv6 addresses wherever you could specify IPv4 ones. IPs and
hostnames are interchangeable everywhere. For certain settings and
commands, you can enclose IPv6 addresses in square brackets to prevent
the colon character (:) from being interpreted as a port separator. These
are documented in the help files and the html documentation, so you can
consult them when in doubt.

---------------------
CTCP CHAT/CHAT4/CHAT6
---------------------

When a user sends a CTCP chat request, the request is passed to the bot via the IRC server, hiding the user's IP. Since Eggdrop is unable to 'see' the type IP of the user is using (IPv4 or IPv6), it is thus unable to determine whether it should send back an IPv4 or an IPv6 address for the user to connect to. To work around this problem, the CHAT4 and CHAT6 commands were added to Eggdrop to force it to present an IPv4 or IPv6 address for use with a DCC connection, respectively. Otherwise, the traditional CHAT command will likely result in the Eggdrop presenting an IPv4 address to the user. So in short, if you're on an IPv6 address and want to use CTCP CHAT to initiate a DCC session for the partyline, use CHAT6, not CHAT as the CTCP argument.

--------
Settings
-------- 

There are four new IPv6 related config variables:

  vhost4

    set this to use a specific vhost with IPv4 connections. Can contain
    either an IP address or a hostname.

  vhost6

    set this to use a specific vhost with IPv6 connections. Can contain
    either an IPv6 address or a hostname.

  prefer-ipv6

    when a connection can be established through both IPv4 and IPv6.
    You can set this to 1 to prefer IPv6 or to 0 to prefer IPv4.

Other affected variables:

  my-ip and my-hostname are removed now. Their function is split between
  vhost4 and the listen command.

  nat-ip works with IPv4 as it used to. It has no meaning for IPv6 and is
  not queried for IPv6 connections.

Copyright (C) 2010 - 2021 Eggheads Development Team
