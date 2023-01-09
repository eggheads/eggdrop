Enabling TLS Security on Eggdrop
================================

There are several ways TLS encryption can protect communication between your Eggdrop and the world. This guide will walk you through a few common scenarios and how to properly set up TLS protection.

Sidenote: Despite SSL (Secure Socket Layer) encryption being deprecated and no longer secure, the term "SSL" is a bit of an anachronism and still commonly used interchangeably with TLS (Transport Layer Security). If you see the term "SSL" used to describe a secure connection method, to include with within Eggdrop's own documentation and configuration files, it is probably safe to assume it is actually referring to the secure TLS protocol. If you talk to someone and they use the term "SSL" be sure to correct them, we're sure they will *definitely* appreciate it :)

Pre-requisites
--------------
Your server must have OpenSSL (or an equivalent fork) installed. Most commonly this is done through your OS's package manager. Both the main package as well as the development headers must be installed. On a Debian/Ubuntu distro, this can be done by running::

  apt-get install openssl libssl-dev

where openssl is the main package binaries, and libssl-dev are the development headers. Without these packages, TLS protection is not possible.

You can check if your Eggdrop properly detected the installation of OpenSSL by either reviewing the ./configure output for the following line::

  checking for openssl/ssl.h... yes

Or, for an Eggdrop that is already running, you can join the partyline and type::

  .status

and look for::

  TLS support is enabled.

Connecting to a TLS-enabled IRC server
--------------------------------------
Many, if not most, IRC servers offer connection ports that add TLS protection around the data. Eggdrop uses a '+' symbol in front of the port to denote a port as a TLS-enabled port. To add a server in the config file that supports TLS, add it as normal but simply prefix the port with a '+'. For example, if irc.pretendNet.org says it offers TLS on port 7000, you would add it to your configuration file as::

  server add irc.pretendNet.org +7000

No other action is necessary.

Protecting Botnet Communications
--------------------------------
Eggdrop has the ability to protect botnet (direct bot to bot) communications with TLS.

Configuration File Preparation - Generating Keys
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If an Eggdrop is going to listen/accept connections on a TLS port (more on that in a moment), it must have a public/private certificate pair generated and configured. For most users, a self-signed certificate is sufficient for encryption (a certificate signed by a certificate authority would be more secure, but obtaining one is outside the scope of this tutorial. However, the implementation of a signed keypair is no different than a self-signed pair). To generate a self-signed key pair, enter the Eggdrop source directory (the directory you first compiled Eggdrop from, usually named eggdrop-X.Y.Z) and type::

  make sslcert

The wizard will walk you through generating a keypair and will, by default, install to ~/eggdrop (the install location can be changed by "make sslcert DEST=/path/to/eggdrop/install"

In your config file, uncomment the "ssl-privatekey" and "ssl-certificate" settings. Eggdrop will look in the directory it is running from (~/eggdrop by default) for the files listed; add an absolute path if you installed them outside of Eggdrop'd directory.

Configuration File Preparation - Listening with TLS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Now you need to tell Eggdrop to accept TLS connections. As an example, to listen with TLS on port 5555 on all available IPs, add to the config file::

  listen +5555 all

(There are numerous ways to format the listen command; read the config file and documentation for other alternatives)

Connecting to an Eggdrop listening with TLS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To connect to a bot listening with TLS, prefix the port number with a '+'. For example::

  .+bot HubBot 1.2.3.4 +5555

will use TLS to connect to 1.2.3.4 on port 5555 the next time a connection is attempted to HubBot.

Additional Information
----------------------
For additional information and a more thorough explanation of Eggdrop's TLS implementation, please read the `TLS docs <https://docs.egheads.org/using/tls.html>`_
