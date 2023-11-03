Last revised: November 03, 2023

.. _python:

============
Python Module
============

This module adds a Python interpreter to Eggdrop, allowing you to run Python scripts.

--------------
Loading Python
--------------

Put this line into your Eggdrop configuration file to load the python module::

  loadmodule python

To load a python script from your config file, place the .py file in the scripts/ folder and add the following line to your config::

  pysource scripts/myscript.py

------------------
Partyline Commands
------------------

You can run a python command from the partyline with the .py command, such as::

  .py print('Hello world!')

----------------
Config variables
----------------

There are also some variables you can set in your config file:

  set allow-resync 0
    When two bots get disconnected, this setting allows them to create a
    resync buffer which saves all changes done to the userfile during
    the disconnect. When they reconnect, they will not have to transfer
    the complete user file, but, instead, just send the resync buffer.

-----------------------
Eggdrop Python Commands
-----------------------

  The Python module is built to use the existing core Tcl commands integrated into Eggdrop via the ``eggdrop`` module. To use 

Copyright (C) 2000 - 2023 Eggheads Development Team
