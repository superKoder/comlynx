# Atari Lynx virtual ComLynx cable

This is a fully functional software implementation of Epix' 1989 UART implmentation of the Atari Lynx (called RedEye or ComLynx) for use in emulators.

In the real Atari Lynx, the ComLynx used the built-in UART, driven by Timer 4, with RX and TX soldered together. The format was forced to 1 start bit, 8 data bits and 1 stop bit. But many baud rates were supported and no formal protocol was defined beyond that.


## Supported games

There was never a formal ComLynx protocol, though [most games appear to be using a very similar way of communication](https://github.com/superKoder/lynx_game_info) which is fully supported. This includes games such as _Warbirds_, _Xenophobe_, _Checkered Flag_, and the 8-player _Slime World_!

One very prominent game that is **not** supported is _California Games_. It communicates in a unique way, and it will lock up as soon as you get through the opening scene. This may eventually be solved...


## How to use this

This project by itself is pretty worthless without an emulator. It has been tested within a heavily modified version of (a the libretro version of) Handy, where it was shown to work. I may release that one later...


## License & re-use

You may use this in your software, but you must retain the copyright and mention that you found it at https://github.com/superKoder/comlynx, in accordance with the MIT license. Thank you!