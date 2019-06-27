Last revised: September 26, 2010

.. _dns:

==========
DNS Module
==========

This module provides asynchronous dns support. This will avoid long periods
where the bot just hangs there, waiting for a hostname to resolve, which will
often let it timeout on all other connections.

This module requires: none

Put this line into your Eggdrop configuration file to load the dns
module::

  loadmodule dns

There are also some variables you can set in your config file:

  set dns-servers "8.8.8.8 8.8.4.4"
    In case your bot has trouble finding dns servers or you want to use
    specific ones, you can set them here. The value is a list of dns servers.
    The relative order doesn't matter. You can also specify a non-standard
    port.
    The default is to use the system specified dns servers. You don't need to
    modify this normally.

  set dns-cache 86400
    Specify how long should the DNS module cache replies at maximum. The
    value must be in seconds.
    Note that it will respect the TTL of the reply and this is just an upper
    boundary.

  set dns-negcache 600
    Specify how long should the DNS module cache negative replies (NXDOMAIN,
    DNS Lookup failed). The value must be in seconds.

  set dns-maxsends 4
    How many times should the DNS module resend the query for a given domain
    if it receives no reply?

  set dns-retrydelay 3
    Specify how long should the DNS module wait for a reply before resending
    the query. The value must be in seconds.


Copyright (C) 2000 - 2019 Eggheads Development Team
