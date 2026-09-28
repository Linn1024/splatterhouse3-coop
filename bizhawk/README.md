# Splatterhouse 3 Co-op in BizHawk

The standalone installer is the easiest way to play. This optional adapter is
for players who prefer BizHawk. It has passed headless boot and save/replay
checks; interactive BizHawk play remains unverified.

## Install the custom core

1. Install the project or extract its [portable release](../README.md#download-and-install).
   Both include the adapter DLLs in `bizhawk/`. If using source code instead,
   [build the project](../DEVELOPING.md#build-the-game) first.
2. Have a separate 64-bit BizHawk installation ready and close it.
3. Open PowerShell in the Splatterhouse 3 Co-op folder and run:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File .\bizhawk\install.ps1 -BizHawkDirectory 'C:\Games\BizHawk'
   ```

   Replace the example path with the folder containing your `EmuHawk.exe`.
4. Start BizHawk and select **File > Open Advanced > Libretro**.
5. Choose `Libretro/Cores/Splatterhouse3Coop/splatterhouse_coop_libretro.dll`
   inside the BizHawk folder, then select your [supported ROM](../README.md#supported-rom).
6. Configure both players under **Config > Controllers**.

The install script copies only the two custom DLLs into their own core folder.
Keep `splatterhouse_engine.dll` beside `splatterhouse_coop_libretro.dll`.

## Controls and saves

Map RetroPad **B** to Attack, **Y** to Transform, **A** to Jump, and **Start**
to Start/Pause. Choose keyboard keys or controllers in BizHawk; no controller
profile is included.

**Genesis X / RetroPad L** defeats enemies in the current room, including bosses.
Press Start to show the map, then Attack + Transform + Jump together to open the
cheat menu. Up/Down selects, Jump applies, Left/Right changes the selected stage,
and Start resumes. See the [main guide](../README.md#optional-cheats).

BizHawk's own save/load commands serialize both players. Use the same custom
core to load these saves; BizHawk saves and the standalone launcher's save file
are not interchangeable.

## Display

The adapter outputs a fixed 320x254 surface: the original 320x224 playfield plus
30 rows for the second player's HUD. P1 is the upper row and P2 the lower row.
The aspect ratio includes the extra height without stretching the playfield.
