----- OVERVIEW AND LINKS -----

This is an improved version of DDRace, a mod for the game teeworlds, with block features, etc. It is provided under the same licence as DDRace and original teeworlds. See xxl-clan.com and github page for up-to-date information.

----- HOW TO USE THIS DOCKER CONTAINER -----

First, pull the latest binary with
   docker pull xxlddrace
Then run it, and forward the docker port 8303 to the external port you want to use, but REMEMBER, in your autoexec.cfg you have to write port 8303. You also have to specify your config directory (containing autoexec.cfg and data directory) as volume, which the docker container can use. If you don't know what to write, please read the docker documentation first.

----- ISSUES AND IMPROVEMENT -----

If you want to suggest new features or report bugs, please do it on xxl-clan.com or on github page.

DDRace README is as follows:
This is a mod (DDRace) and it's for the game Teeworlds and it's being maintained by GreYFoX@GTi & btd with the help of others like heinrich5991, noother & Floff, in the previous versions 3da and Fluxid.

DDRace has been rebuilt from scratch by btd based on DDRace-Beta to Teeworlds Trunk 0.5, GreYFoX@GTi helped re-adding features once it was Trunk, you can track this in the commit log.

Please visit http://www.DDRace.info for up-to-date information about 
DDRace, including new versions, custom maps and much more.

Teeworlds README is as follows:
Copyright (c) 2012 Magnus Auvinen


This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.


Please visit http://www.teeworlds.com for up-to-date information about 
the game, including new versions, custom maps and much more.
