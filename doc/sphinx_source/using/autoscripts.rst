Eggdrop Autoscripts
===================

Since it's inception, users have needed to load a Tcl script into Eggdrop by downloading a Tcl file, editing the file to customize settings, and then sourceing that file in the config file. In v1.10, the Autoscripts system was added to make this process a little more user-friendly. The autoscripts system helps by:

* Centralizing commonly-used scripts in a single location
* Allowing scripts to be downloaded via the partyline
* Allowing script settings to be configured via the partyline
* Allowing user-written scripts to be managed by the autoscripts system
* Providing a documented API to write autoscripts-compatible scripts

Autoscripts usage
-----------------
To view available autoscript commands, type ``.autoscript`` on the partyline. This will open up a special Eggdrop console that doesn't require you to prefix commands with a '.' . The following sub-commands are available for use with script:

remote
^^^^^^
This command will list scripts hosted on the Eggdrop website that are available to be downloaded and installed on your Eggdrop.

fetch <script>
^^^^^^^^^^^^^^
This command will download the specified script from the Eggdrop website and place it into the autoscript/ directory.

list
^^^^
This command will list scripts locallt present in the autoscripts/ directory, available to be configured and loaded.

config <script>
^^^^^^^^^^^^^^^
This command will list settings available for configuration for the provided script.

set <script> <setting>
^^^^^^^^^^^^^^^^^^^^^^
This command will set ``setting`` for ``script`` to the provided value. To activate this change, use the ``load`` command.

load <script>
^^^^^^^^^^^^^
This command will activate the script for use. You can also use this command to reload a script after modifying a script variable.

unload <script>
^^^^^^^^^^^^^^^
This command will prevent the script from being loaded the next time Eggdrop starts. To fully unload a script, Eggdrop must be restarted!

clean <script>
^^^^^^^^^^^^^^
This command will delete the script from the filesystem. After running this command, you will have to re-download and re-configure the script if you wish to use it again.

update [script]
^^^^^^^^^^^^^^^
If no script is specified, this command checks if there any downloaded script has a newer version available. If a script is specified, autoscript will fetch and install the updated script.


Autoscripts File Structure
--------------------------
An autoscripts package requires (minimum) two files: the Tcl script, and a json manifest file. 

Tcl File
^^^^^^^^
Nothing new or novel here; this is where your Tcl code goes. The one change to this file is that any setting intended should now be located in the manifest.json file, not the Tcl script file. All variables will be added to the global namespace. For this reason, it is recommended that variables have unique names to avoid collisions. This is commonly done by prefixing the variable name with the script name or abbreviation. For example, a script called myscript.tcl might avoid using a variable called ``name`` and instead use ``myscript_name`` or ``ms_name``.

Manifest.json
^^^^^^^^^^^^^
Every autoscripts package must have a manifest.json file. This file contains metadata for the script such as version and description information, as well as the user-configurable settings for use with th script. A simple example of a manifest.json file is as follows::

  {
    "schema": 1,
    "name": "woobie",
    "version_major": 1,
    "version_minor": 0,
    "description": "An example script to help developers write autoscript packages",
    "long_description": "This is an example script to help understand the autoscript system. Yeah, it doesn't really do anything, but that's besides the point. It could, and that should be enough for anyone"
    "config": {
      "loaded": 0,
      "udefflag":"myscript"
      "requires": "tls",
      "vars": {
        "woobie_dict": {
          "description": "A setting that accepts a dict as a value",
          "value": "{quiet q}"
        },
        "woobie_setting": {
          "description": "A normal setting to enable or disable something",
          "value": "1"
        },
        "woobie_string": {
          "description": "A setting taking a string, like a filename or something",
          "value": "woobie"
        },
        "woobie(array)": {
          "description": "A setting that is set as an array",
          "value":"another string"
        }
      }
    }
  }

+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| schema                            | The schema version of autoscript (currently 1)                                                                                                                                                                                                                         |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| name                              | The name of the script. Must match the script name (if the script is foo.tcl, then this must be foo)                                                                                                                                                                   |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| version_major                     | The major version integer (ie, 1 for 1.6)                                                                                                                                                                                                                              |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| version_minor                     | The minor version integer (ie, 6 for 1.6)                                                                                                                                                                                                                              |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| description                       | A one-line summary of what the script does. This will be shown when available scripts are listed on the partyline via .script list.                                                                                                                                    |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| long_description                  | A longer description of what the script does, similar to a README. This will be shown when a script is viewed via .script config.                                                                                                                                      |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| config-loaded                     | Whether this script is currently loaded or not. It should be default set to 0.                                                                                                                                                                                         |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| config-udefflag                   | Any user-defined channel settings used by the script. This is displayed when configuration settings are displayed to the user on the partyline.                                                                                                                        |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| config-requires                   | Any Tcl package required for use by the script, such as tls, http, json, etc.                                                                                                                                                                                          |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| config-vars-<varname>             | A setting intended to be modified by the user. The 'description' field should describe what the setting does, and the 'value' field stores the current value. These settings are displayed when the configuration settings are displayed to the user on the partyline. |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| config-vars-<varname>-description | A description of the setting, displayed in the configuration listing for the script.                                                                                                                                                                                   |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| config-vars-<varname>-value       | The value the setting is set to                                                                                                                                                                                                                                        |
+-----------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

File placement
^^^^^^^^^^^^^^
Autoscript files are stored in the autoscript directory. The path structure is eggdrop/autoscript/<scriptname>/[script files]. If the autoscript ``fetch`` command is used, a .tgz file will be downloaded and extracted to the proper location automatically. If you wish to manually add a script, create a directory with the same name as the script, and then place the script and manifest files inside the directory. The directory name must exactly match the script name (without the .tcl extension)! If the Tcl script to be loaded is called ``myscript_goodversion_specialfeature.tcl``, then the directory must also called ``myscript_goodversion_specialfeature``.

Development hints
-----------------

* An autoscript should not require a user to manually open the script in an editor for any reason. Design your script as such!
* Use `user defined channel flags <https://docs.eggheads.org/using/tcl-commands.html#setudef-flag-int-str-name>`_ to enable/disable a script for a particular channel, they're easy!
* Variables used in autoscripts are placed into the global namespace. Make them unique to prevent collisions! We recommend prefixing the script name in front of a variable, such as myscript_setting or ms_setting.

Tcl Commands
------------

The autoscripts Tcl script adds three new commands for use with Tcl scripts:

egg_loaded
^^^^^^^^^^

  Description: lists all scripts currently loaded via the autoscripts system

  Returns: A Tcl list of script names currently loaded via autoscripts

egg_unloaded
^^^^^^^^^^^^

  Description: lists all scripts downloaded to the local machine via the autoscripts system but not currently loaded by Eggdrop

  Returns: A Tcl list of script names downloaded but not currently loaded via autoscripts

egg_all
^^^^^^^

  Description: lists all script downloaded to the localm machine via the autoscripts system, regardless if they are running or not

  Returns: A Tcl list of all script namees download via autoscripts
