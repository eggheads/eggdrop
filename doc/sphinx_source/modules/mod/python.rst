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

^^^^^^^^^^^^^^^^^^^
python <expression>
^^^^^^^^^^^^^^^^^^^

You can run a python command from the partyline with the .python command, such as::

  .python print('Hello world!')

^^^^^^^^^^^^^
.binds python
^^^^^^^^^^^^^

The python module extends the core ``.binds`` command by adding a ``python`` mask. This command will list all binds for python scripts.

------------
Tcl Commands
------------

^^^^^^^^^^^^^^^^^^^^^^^
pysource <path/to/file>
^^^^^^^^^^^^^^^^^^^^^^^

The ``pysource`` command is analgous to the Tcl ``source`` command, except that it loads a Python script into Eggdrop instead of a Tcl script.

---------------
Python Commands
---------------

^^^^^^^^^^^^^^^^^^^^^^^
parse_tcl_list <string>
^^^^^^^^^^^^^^^^^^^^^^^

When a python script calls a Tcl command that returns a list via the eggdrop.tcl module, the return value will be a Tcl-formatted list- also simply known as a string. The ``parse_tcl_list`` command will convert the Tcl-formatted list into a Python list, which can then freely be used within the Python script.

^^^^^^^^^^^^^^^^^^^^^^^
parse_tcl_dict <string>
^^^^^^^^^^^^^^^^^^^^^^^

When a python script calls a Tcl command that returns a dict via the eggdrop.tcl module, the return value will be a Tcl-formatted dict- also simply known as a string. The ``parse_tcl_dict`` command will c
onvert the Tcl-formatted dict into a Python list, which can then freely be used within the Python script.

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

--------------------------------
Writing an Eggdrop Python script
--------------------------------

  This is how to write a python script for Eggdrop. 

^^^^^^^^^^^^^^
Header section
^^^^^^^^^^^^^^

An Eggdrop python script requires you to import X Y and Z, in this format. 

^^^^^^^^^^^^
Code Section
^^^^^^^^^^^^

Normal python code works here. To run a command from the Eggdrop Tcl library, use this format.

Use this format all over.

-------------------------------------
Writing a module for use with Eggdrop
-------------------------------------

This is how you import a module for use with an egg python script.


Copyright (C) 2000 - 2023 Eggheads Development Team
