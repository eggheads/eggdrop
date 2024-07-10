Last revised: November 03, 2023

.. _python:

=============
Python Module
=============

This module adds a Python interpreter to Eggdrop, allowing you to run Python scripts.

-------------------
System Requirements
-------------------
This module requires Python version 3.8 or higher in order to run. Similar to Tcl requirements, Eggdrop requires both python and python development libraries to be installed on the host machine. On Debian/Ubuntu machines, this means the packages ``python``, ``python-dev`` AND ``python-is-python3`` to be installed. The python-is-python3 updates symlinks on the host system that allow Eggdrop to find it.

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

  .python 1 + 1
  Python: 2
  .python from eggdrop.tcl import putmsg; putmsg('#chan', 'Hello world!')
  Python: None

^^^^^^^^^^^^^
.binds python
^^^^^^^^^^^^^

The python module extends the core ``.binds`` partyline command by adding a ``python`` mask. This command will list all binds for python scripts.

------------
Tcl Commands
------------

^^^^^^^^^^^^^^^^^^^^^^^
pysource <path/to/file>
^^^^^^^^^^^^^^^^^^^^^^^

The ``pysource`` command is analogous to the Tcl ``source`` command, except that it loads a Python script into Eggdrop instead of a Tcl script.
