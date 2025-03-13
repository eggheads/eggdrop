Sharing Userfiles
=================

One of the great features of Eggdrop is bot linking, allowing you to create your own 'botnet'. A botnet can consist of two linked bots to as many as hundreds of linked bots. If you're using an Eggdrop for channel management, the most important feature of linked bots is userfile sharing. With userfile sharing, the bots can keep their user lists, ban lists, and ignore lists in sync with one another. A botnet also has many other possible capabilities, such as allowing bots to op one another securely, and making all bots join a channel with one command.

Linking bots can be a confusing process for first timers, but it's quite simple if you know how. The first thing you need to do is chose a hub bot. The hub is the main bot which all the other bots on the botnet (which will become leaf bots) connect to. Even if you are only using two bots, it's best to use one as a hub bot, and the other as a leaf. The hub bot should be the most reliable bot on your botnet, running on a fast, stable, and secure shell - so keep that in mind when deciding which bot will be the hub.

Once you've decided on the hub, it's time to set up the linking.

How to share userfiles- the super-short version
-----------------------------------------------

(This example steps assume Eggdrop has been compiled with TLS support, and you are sharing all channels)

On the Hub Bot
^^^^^^^^^^^^^^

#. Ensure you know what ports your bot is listening on, this is set in the config and those ports will be used in the example below. For this example, our config looks like::

    listen +3333/+4444 all

#. On the hub (for this example, the Hub is called Hubalicious), add the leaf bot (for this example, the Leaf is called LeifErikson) with ``.+bot LeifErikson <IP_address_of_LiefErikson> +4444/+3333 <LiefErikson_hostmask>``. 

#. On the hub, give the leaf bot the appropriate leaf sharing flags with ``.botattr LiefErikson +gs``.

On the Leaf Bot
^^^^^^^^^^^^^^^

#. Ensure you know what ports your bot is listening on, this is set in the config and those ports will be used in the example below. For this example, our config looks like::

    listen +5555/+6666 all

#. Now on the leaf, add the hub bot with ``.+bot Hubalicious <IP_address_of_Hubalicious> +6666/+5555 <Hubalicious hostmask>``.

#. On the leaf, give the hub bot the appropriate hub and sharing flags with ``.botattr Hubalicious +ghp``.

At this point, the leaf bot should attempt to connect to the hub bot within the next minute, or you can force the link connection with ``.link Hubalicious``. You can also use .bottree to see your botnet structure.

Explaining the Linking/Sharing Process
--------------------------------------

Eggdrop bots can talk to each other for a variety of reasons. In order for an Eggdrop to talk to another Eggdrop for anything, they must link. This is done by adding a bot record the remote bot on the hub bot (In our example above, using ``.+bot LeifErikson`` on Hubalicious, and using ``.+bot Hubalicious`` on LiefErikson). Once the bot records are added, bots can be manually connected using the ``.link`` command.

In the example above, we add the +s bot flag to LiefErikson's bot record on Hubalicious to tell Hubalicious that LiefErikson is not only allowed to connect, but is authorized to download the userfiles from Hubalicious. Similarly, we add the +p bot flag to Hubalicious's bot record on LiefErikson to tell LiefErikson that Hubalicous is authorized to send userfiles to LiefErikson. The +h bot flag is added to Hubalicious's bot record on LiefErikson so tell LiefErikson that Hubalicious is the hub, and it should always try to automatically connect to Hubalicious.

Lastly, the +g flag is used on both bot records to indicate that Hubalicious is authorized to send userfiles for all channels. This is a shortcut method to sharing all channels instead of setting the ``+shared`` channel setting on the hub for each channel userfile you wish the hub to share (set via .chanset), and using the ``|+s #channel`` bot flag for each channel userfile that the leaf is authorized to receive userfiles from the hub. As an example of channel-specific userfile sharing, you would use ``.botattr LiefErikson |+s #theforest`` on Hubalicious to set #theforest as a channel authorized to be shared to LiefErikson, and ``.chanset #theforest +shared`` to tell LiefErikson to accept the channel userfile for #theforest.

One more commonly used flag is the ``+a`` flag. This flag specifies an alternate hub to connect to in case the primary hub goes down. The alternate hub should be linked to hub and maintain all channel userfiles the hub maintains to ensure there is no desynchronization while the hub bot is down. Should the hub bot go down, and assuming the ``+h`` flag is set for the hub on a leaf, the leafbost will automatically try to reconnect to the hub every minute, even if it makes a connection with the alternate hub in the meantime. An alternate hub is added the same as a hub, except swapping out the ``h`` for the ``a``: ``.botattr AltRock +agp``.

Now that you have a hub and leaf bot successfully connected and sharing userfiles, you can repeat the `On the Leaf Bot`_ section to add additional leafs to your hub.
