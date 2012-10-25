XCAPE
=====

xcape allows you to use a modifier key as another key when pressed and
released on its own. Note that it is slightly slower than pressing the
original key, because the pressed event does not occur until the key is
released. The default behaviour is to generate the Escape key when Left
Control is pressed and released on its own. (If you don't understand why
anybody would want this, I'm guessing that Vim is not your favourite text
editor ;)

Minimal building instructions
-----------------------------

    $ sudo apt-get install git gcc make libx11-dev libxtst-dev
    $ mkdir xcape
    $ cd xcape
    $ git clone https://github.com/alols/xcape.git .
    $ make

Usage
-----
    $ xcape [-d] [-t <timeout ms>] [-e <map-expression>]

### `-d`

Debug mode. Does not fork into the background.

### `-t <timeout ms>`

If you hold a key longer than this timeout, xcape will not generate a key
event. Default is 50 ms.

### `-e <map-expression>`

The expression has the grammar `'ModKey=Key[|OtherKey][;NextExpression]'`

The list of key names is found in the header file `X11/keysymdef.h`
(remove the `XK_` prefix).

#### Example

    xcape -e 'Shift_L=Escape;Control_L=Control_L|O'

This will make Left Shift generate Escape when pressed and released on
it's own, and Left Control generate Ctrl-O combination when pressed and
released on it's own.

Contact
-------

Find the latest version at
https://github.com/alols/xcape

The author can be reached at
albin dot olsson at gmail dot com
