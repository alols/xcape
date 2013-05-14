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

    $ sudo apt-get install git gcc make libx11-dev libxtst-dev pkg-config
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

Alternatively, you can specify ModKey in decimal (prefix `#`), octal (`#0`), or
hexadecimal (`#0x`). It will be interpreted as a keycode unless no corresponding
key name is found.

#### Examples

1) This will make Left Shift generate Escape when pressed and released on
it's own, and Left Control generate Ctrl-O combination when pressed and
released on it's own.

    xcape -e 'Shift_L=Escape;Control_L=Control_L|O'

2) If your `s` key has the code `42` and your `l` key `43` and you have set both
to `AltGr` (a.k.a. ISO_Level3_Shift) with xmodmap, then this will generate the
ordinary letters when pressed and released on their own. But pressed together
with another key, the `s` or `l` key will produce `AltGr`. So, depending on your
keyboard layout, you can compose e.g. `@`, `{` or `³` easily when touch-typing.

    xcape -e '#42=s;#43=l'

Note regarding xmodmap
----------------------

If you are in the habit of remapping keycodes to keysyms (eg, using xmodmap),
there are two issues you may encounter.

1) You will need to restart xcape after every time you modify the mapping from
   keycodes to keysyms (eg, with xmodmap), or xcape will still use the old
   mapping.
   
2) The key you wish to send must have a defined keycode. So for example, with
   the default mapping `Control_L=Escape`, you still need an escape key defined
   in your xmodmap mapping. (I get around this by using 255, which my keyboard
   cannot send).

Contact
-------

Find the latest version at
https://github.com/alols/xcape

The author can be reached at
albin dot olsson at gmail dot com
