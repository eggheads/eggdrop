Eggdrop Module Information
Last revised: Jul 25, 2016

Eggdrop Module Information
==========================

What are modules?
-----------------

Modules are independent portions of code that are loaded separately from the main core code of Eggdrop. This allows users to only implement the features they desire without adding the extra overhead or "bloat" of those they don't, or even write their own module to add an enhancement not currently implemented by the Eggdrop development team. For example, the transfer module provides the ability to transfer files to and from the Eggdrop, and the ident module provides the ability to run an ident server to answer ident requests.

How to install a module
-----------------------

Please note that these are only basic instructions for compiling and installing a module. Please read any and all directions included with the module you wish to install.

  1. Download and un-tar the Eggdrop source code.

  2. Place the new module in its own directory (in the format of
     (modulename).mod) in src/mod.

  3. Run ./configure (from eggdrop1.9.x/).

  4. Type 'make config' or 'make iconfig'.

  5. Type 'make'.

  6. Copy the compiled module file (modulename.so) into your bot's
     modules folder.

  7. Add 'loadmodule modulename' to your eggdrop.conf file (do not
     add the .so suffix).

  8. Rehash or restart your bot.

To view your currently loaded modules, type '.module'.

Can I compile Eggdrop without dynamic modules? (Static compile)
---------------------------------------------------------------
Yes, you can. If the configure script detects that your system CAN'T run modules, it will setup 'make' to link the modules in statically for you. You can choose this option yourself by using 'make static'. You can also try to compile dynamic modules on a static-only system by using 'make eggdrop'.

Do I still need to 'loadmodule' modules?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

YES, when you compile statically, all the modules are linked into the main executable. HOWEVER, they are not enabled until you use loadmodule to enable them, hence you get nearly the same functionality with static modules as with dynamic modules.

Copyright (C) 1999 - 2023 Eggheads Development Team
