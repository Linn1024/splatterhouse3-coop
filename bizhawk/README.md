# Splatterhouse 3 Co-op in BizHawk

The adapter follows the existing local Aladdin setup. It has been tested through
a headless Libretro host; interactive BizHawk operation remains to be checked.

1. Run `build.ps1` in the project root.
2. Run `bizhawk/install.ps1 -BizHawkDirectory C:\TEMP2\Bizhawk`.
3. In BizHawk, select File > Open Advanced > Libretro.
4. Choose `Libretro/Cores/Splatterhouse3Coop/splatterhouse_coop_libretro.dll`
   and the supported `Splatterhouse 3 (USA).md` ROM.
5. Configure both players under Config > Controllers.

Keep `splatterhouse_engine.dll` beside the adapter. The install script copies
only these two files into their own core directory.

Map RetroPad B to Attack, Y to Transform, A to Jump, and Start to Start/Pause.
BizHawk's own save/load commands serialize both players through this adapter.
Use the same custom core to load its saves.

See the [project README](../README.md) for the current prototype limitations.

## Pause/debug menu

**Genesis X / RetroPad L** kills enemies in the current room during gameplay.
The current keyboard bindings are **V** for P1 and **U** for P2. Keyboard **X**
is bound to Attack; these controls are separate. Boss deaths retain native
stage progression.

Press Start to show the native map, then the three Genesis action buttons together (Attack + Transform
+ Jump). Up/Down selects, Jump applies, Left/Right changes the selected stage,
and Start resumes. See the project README for the full cheat list. Use the
project's **Play Splatterhouse 3 in BizHawk** shortcut to launch the installed
core with the existing controller configuration.

## Stable display size

The adapter outputs a fixed 320x254 surface. The game retains its previous
320x224 presentation, with 30 extra rows underneath for P2's original-style
POW/LIFE panel. P1 is the upper row; blue/red labels identify the players.
The aspect ratio includes the extra height without stretching the playfield.
Native 256- and 320-dot modes share this fixed surface, so menu/gameplay
transitions do not resize BizHawk's window. Non-gameplay scenes leave the
additional strip black. Save files retain the original native frame dimensions
and remain compatible. `tools/verify_display_size.py`
checks both native modes, stage transitions, debug, reset and save/load.
