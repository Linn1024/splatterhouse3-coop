# Splatterhouse 3 Co-op in BizHawk

The standalone installer is the easiest way to play. This optional adapter is
for players who prefer BizHawk. It has passed headless boot and save/replay
checks; interactive BizHawk play remains unverified.
Co-op runs in the custom emulator core. No ROM patch or Lua script is needed;
opening the ROM with BizHawk's stock Genesis core runs the single-player game.

## Install the custom core

1. Install the project or extract its [portable release](../README.md#download-and-install).
   Both include the adapter DLLs in `bizhawk/`. If using source code instead,
   [build the project](../DEVELOPING.md#build-the-game) first.
   From the source folder containing `build.ps1`, with a Windows x64 MinGW-w64
   toolchain providing `gcc`, `g++`, and `mingw32-make`, run:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -ToolchainBin 'C:\Tools\mingw64\bin'
   ```

   Replace the compiler path, or omit `-ToolchainBin` if the tools are on `PATH`.
   Skip this build step when using the installer or portable release.
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
7. Press your mapped Start button at the title screen and begin a new game.
   During gameplay, check that both Ricks appear with separate HUD rows and
   that each player's movement and attack controls respond.

The install script copies only the two custom DLLs into their own core folder.
Keep `splatterhouse_engine.dll` beside `splatterhouse_coop_libretro.dll`.
For a manual install, copy both DLLs from the project's `bizhawk` folder into
the layout below. Do not select `splatterhouse_engine.dll` as the frontend core.

```text
BizHawk/
  EmuHawk.exe
  Libretro/Cores/Splatterhouse3Coop/
    splatterhouse_coop_libretro.dll
    splatterhouse_engine.dll
```

The ROM can stay elsewhere and can have any filename when selected manually.
Its contents must match the USA ROM checksum in the main guide.

## Controls and saves

Map RetroPad **B** to Attack, **Y** to Transform, **A** to Jump, and **Start**
to Start/Pause. Choose keyboard keys or controllers in BizHawk; no controller
profile is included.

Suggested bindings for **each player's RetroPad**:

| RetroPad control | Game action | P1 keyboard | P2 keyboard |
| --- | --- | --- | --- |
| D-pad | Move | WASD | Arrow keys |
| B | Attack | F | J |
| Y | Transform | G | K |
| A | Jump | Space | L |
| Start | Start / map | Enter | Right Shift |
| L | Defeat room enemies (cheat) | X | O |

These are suggestions, not an installed profile. BizHawk's RetroPad labels differ
from Genesis button labels. Set emulator pause, reset, and save/load shortcuts
under **Config > Hotkeys**, avoiding keys assigned to gameplay. The standalone
launcher's function-key shortcuts do not configure BizHawk.

**Genesis X / RetroPad L** defeats enemies in the current room, including bosses.
Press Start to show the map, then Attack + Transform + Jump together to open the
cheat menu. Up/Down selects, Jump applies, Left/Right changes the selected stage,
and Start resumes. See the [main guide](../README.md#optional-cheats).

BizHawk's own save/load commands serialize both players. Use the same custom
core to load these saves; BizHawk saves and the standalone launcher's save file
are not interchangeable.
Back up saves before changing core versions; compatibility across prototype
versions is not guaranteed.

## Updates and troubleshooting

Close BizHawk, update or rebuild the project, rerun `bizhawk\install.ps1`, and
restart BizHawk. Install both DLLs from the same build together. The install
script does not download or build the DLLs itself.

- **Only one Rick:** open through **File > Open Advanced > Libretro** and choose
  `splatterhouse_coop_libretro.dll`, then select the ROM as content.
- **Missing DLL / core cannot load:** check the two-file layout above and use
  Windows x64 BizHawk. Extract the full portable release or finish the source
  build before running the install script.
- **ROM rejected:** verify the [supported ROM checksum](../README.md#supported-rom).
  Renaming a different ROM revision does not make it compatible.
- **P2 does not move:** configure P2 RetroPad separately and resume emulation.
- **Cheat menu will not open:** press the game's Start button to show the map,
  then Attack + Transform + Jump. Emulator pause stops input processing.
- **DLL copy fails:** close any running BizHawk instance using this core and
  check that your BizHawk installation folder is writable before retrying.

No local `.ini` profile or developer shortcut is required for these steps.

## Display

The adapter outputs a fixed 320x254 surface: the original 320x224 playfield plus
30 rows for the second player's HUD. P1 is the upper row and P2 the lower row.
The aspect ratio includes the extra height without stretching the playfield.
