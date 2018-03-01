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

First install the development dependencies. On Debian-based systems
(including Ubuntu and Linux Mint), run:

    $ sudo apt-get install git gcc make pkg-config libx11-dev libxtst-dev libxi-dev

On Fedora-based systems, run:

    $ sudo dnf install git gcc make pkgconfig libX11-devel libXtst-devel libXi-devel

Then run:

    $ git clone https://github.com/alols/xcape.git
    $ cd xcape
    $ make
    $ sudo make install

Usage
-----
    $ xcape [-d] [-f] [-t <timeout ms>] [-e <map-expression>]

### `-d`

Debug mode. Does not fork into the background. Prints debug information.

### `-f`

Foreground mode. Does not fork into the background.

### `-t <timeout ms>`

If you hold a key longer than this timeout, xcape will not generate a key
event. Default is 500 ms.

### `-e <map-expression>`

The expression has the grammar `'ModKey=Key[|OtherKey][;NextExpression]'`

The list of key names is found in the header file `X11/keysymdef.h` (remove
the `XK_` prefix). Note that due to limitations of X11 shifted keys *must*
be specified as a shift key followed by the key to be pressed rather than
the actual name of the character. For example to generate "{" the
expression `'ModKey=Shift_L|bracketleft'` could be used (assuming that you
have a key with "{" above "[").

You can also specify keys in decimal (prefix `#`), octal (`#0`), or
hexadecimal (`#0x`). They will be interpreted as keycodes unless no corresponding
key name is found.

#### Examples

+   This will make Left Shift generate Escape when pressed and released on
    its own, and Left Control generate Ctrl-O combination when pressed and
    released on its own.

        xcape -e 'Shift_L=Escape;Control_L=Control_L|O'

+   In conjunction with xmodmap it is possible to make an ordinary key act
    as an extra modifier. First map the key to the modifier with xmodmap
    and then the modifier back to the key with xcape. However, this has
    several limitations: the key will not work as ordinary until it is
    released, and in particular, *it may act as a modifier unintentionally if
    you type too fast.* This is not a bug in xcape, but an unavoidable
    consequence of using these two tools together in this way.
    As an example, we can make the space bar work as an additional ctrl
    key when held (similar to
    [Space2ctrl](https://github.com/r0adrunner/Space2Ctrl)) with the
    following sequence of commands.

        # Map an unused modifier's keysym to the spacebar's keycode and make it a
        # control modifier. It needs to be an existing key so that emacs won't
        # spazz out when you press it. Hyper_L is a good candidate.
        spare_modifier="Hyper_L"
        xmodmap -e "keycode 65 = $spare_modifier"
        xmodmap -e "remove mod4 = $spare_modifier" # hyper_l is mod4 by default
        xmodmap -e "add Control = $spare_modifier"

        # Map space to an unused keycode (to keep it around for xcape to
        # use).
        xmodmap -e "keycode any = space"

        # Finally use xcape to cause the space bar to generate a space when tapped.
        xcape -e "$spare_modifier=space"


Note regarding xmodmap
----------------------

If you are in the habit of remapping keycodes to keysyms (eg, using xmodmap),
there are two issues you may encounter.

1. You will need to restart xcape after every time you modify the mapping from
   keycodes to keysyms (eg, with xmodmap), or xcape will still use the old
   mapping.

2. The key you wish to send must have a defined keycode. So for example, with
   the default mapping `Control_L=Escape`, you still need an escape key defined
   in your xmodmap mapping. (I get around this by using 255, which my keyboard
   cannot send).

Contact
-------

Find the latest version at
https://github.com/alols/xcape

The author can be reached at
albin dot olsson at gmail dot com
