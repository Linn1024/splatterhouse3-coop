# Source and notices

The `engine/` directory contains modified Genesis Plus GX sources. Original
project: https://github.com/ekeeke/Genesis-Plus-GX . The exact upstream revision
of the imported source snapshot was not recorded.

Retain [engine/LICENSE.txt](engine/LICENSE.txt), the original copyright headers,
and the component-specific license files throughout the source tree. The engine
license includes noncommercial redistribution conditions and a requirement to
provide complete source for modified builds; this repository retains the engine
source and its bundled dependencies. Individual components have their own terms.
No blanket MIT/GPL license is asserted for the combined project.

This project derives from Aladdin Co-op. Its
original Aladdin hooks remain disabled. Splatterhouse behavior is implemented
in `engine/core/m68k/splatterhouse_coop.h`, with engine integration and probe
exports. Adapted frontends are `launcher.cpp` and
`bizhawk/splatterhouse_libretro.cpp`, sharing `coop_hud.h`.

Game ROMs must not be included in a source or binary distribution. Players supply the supported ROM.
Game saves, disassemblies, emulator profiles, and debug captures are excluded.

Binary packages include the complete matching source and component license
files in `source.zip`. The release source revision is recorded in `SOURCE.txt`.
