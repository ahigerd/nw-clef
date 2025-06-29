nw-clef
=======

nw-clef is a player for game music originally published using the RSEQ engine and later variants.

Building
--------
To build on POSIX platforms or MinGW using GNU Make, simply run `make`. The following make
targets are recognized:

* `cli`: builds the command-line tool. (default)
* `plugins`: builds all plugins supported by the current platform.
* `all`: builds the command-line tool and all plugins supported by the current platform.
* `debug`: builds a debug version of the command-line tool.
* `audacious`: builds just the Audacious plugin, if supported.
* `winamp`: builds just the Winamp plugin, if supported.
* `foobar`: builds just the Foobar2000 plugin, if supported.
* `aud_nw-clef_d.dll`: builds a debug version of the Audacious plugin, if supported.
* `in_nw-clef_d.dll`: builds a debug version of the Winamp plugin, if supported.

The following make variables are also recognized:

* `CROSS=mingw`: If building on Linux, use MinGW to build Windows binaries.
* `CROSS=msvc`: Use Microsoft Visual C++ to build Windows binaries, using Wine if the current
  platform is not Windows. (Required to build the Foobar2000 plugin.)
* `WINE=[command]`: Sets the command used to run Wine. (Default: `wine`)

To build using Microsoft Visual C++ on Windows without using GNU Make, run `buildvs.cmd`,
optionally with one or more build targets. The following build targets are supported:

* `cli`: builds the command-line tool. (default)
* `plugins`: builds the Winamp and Foobar2000 plugins.
* `all`: builds the command-line tool and the Winamp and Foobar2000 plugins.
* `winamp`: builds just the Winamp plugin.
* `foobar`: builds just the Foobar2000 plugin.

Separate debug builds are not supported with Microsoft Visual C++, but the build flags may be
edited in `msvc.mak`.

License
-------
nw-clef is copyright (c) 2024 Adam Higerd and distributed under the terms of the
[MIT license](LICENSE.md).

This project is based upon libclef, copyright (c) 2020-2024 Adam Higerd and distributed
under the terms of the [MIT license](LICENSE.md).

Special Thanks
--------------
Thanks go to Ruben Nunez and Valley Bell for their past work, which was instrumental in getting
this project started.
