Repo for "destroyed or abandoned" dockers of Window Maker

are:

wmwave: show wavelan connection aka wireless status, runs ok

wmtime.app: version of clock "wmdate" for wmaker, hacked to show Internet Time

"Swatch" and beats, re "hacked" to runs ok.

wmtimer: is a dockable alarm clock.

wmeyes: xeyes in a docker "the most useless docker of Window Maker"

pclock-bezier: A two-hour hack. Combined the rather cool Bezier Clock (bclock-1.0) with those wonky curves that bezier between the three hands, with pclock-0.10 (above), so the final result is a bezierclock-windowmaker-dockapp

"hacked" to run ok.

Window Maker 0.17.5:

Runs ok in 2015, only changues, re formated Makefiles, and configure to actual format, install without problems :)

remember backup the GNUstep dir, if uses a never version of wmaker...

Installation:

In the wmaker dir:

$ cd WindowMaker-0.17.5

$ cd libProplist

$ ./configure

$ make

$ cd..

$ ./configure

$ make

Runs with root: make install 

run this in root user with su or sudo commands.

"optionally" # make uninstall before install this very old wmaker..

runs wmaker.inst script, and enjoy :)

Screenshot of this older wmaker: http://i.imgur.com/prNPBkP.jpg

PD: Added very older Window Maker pixmaps, first appeared in wmaker 0.6.3