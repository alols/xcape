XCAPE
=====

xcape runs as a daemon and intercepts the Control key. If the 
Control key is pressed and released on its own, it will generate an Escape
key event.

This makes more sense if you have remapped your Caps Lock key to Control.
Future versions of this program might do that mapping for you, but for now
this is something that you have to do yourself.

If you don't understand why anybody would want this, I'm guessing that Vim
is not your favourite text editor ;)

Minimal building instructions
-----------------------------

    $ sudo apt-get install git gcc make libx11-dev libxtst-dev
    $ mkdir xcape
    $ cd xcape
    $ git clone https://github.com/alols/xcape.git .
    $ make

Usage
-----
    $ xcape -e 'Shift_L=Shift_L|parenleft;Shift_R=Shift_R|parenright;Control_L=Escape'

Will make Left Shift pressed alone generate (, Right shift to generate ) and 
Left Control generate Escape.

Contact
-------

Find the latest version at
https://github.com/alols/xcape

The author can be reached at
albin dot olsson at gmail dot com
