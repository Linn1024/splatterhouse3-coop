# Splatterhouse 3 Co-op

Playable prototype using the emulator-assisted approach from Aladdin Co-op.

## Install on Windows (64-bit)

This repository contains source code. **Code > Download ZIP does not include
the executable, emulator DLLs, or game ROM.** Build the project once using the
steps below. The standalone launcher does not require BizHawk or Python.

### 1. Get the source and compiler

Download [the source ZIP](https://github.com/Linn1024/splatterhouse3-coop/archive/refs/heads/main.zip)
and extract it to a writable folder, for example `C:\Games\splatterhouse3-coop`.
Open the extracted folder containing `build.ps1`; do not run from inside the ZIP.
Alternatively, with Git installed:

```powershell
git clone https://github.com/Linn1024/splatterhouse3-coop.git
cd splatterhouse3-coop
```

Install a **64-bit MinGW-w64 GCC toolchain**, such as a Win64/x86_64 ZIP from
[WinLibs](https://winlibs.com/). Extract it, for example to `C:\Tools\mingw64`.
Its `bin` folder must contain `gcc.exe`, `g++.exe`, and `mingw32-make.exe`.
Use your actual extracted path in the next command.

### 2. Build

Open PowerShell in the project folder and run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -ToolchainBin "C:\Tools\mingw64\bin"
```

Always supply `-ToolchainBin`: the script's default points to the developer's
local compiler. The execution-policy option applies only to this PowerShell
process. A successful build creates:

```text
Splatterhouse3-Coop.exe
engine/genesis_plus_gx_libretro.dll
bizhawk/splatterhouse_coop_libretro.dll
bizhawk/splatterhouse_engine.dll
```

### 3. Add your ROM

Supply your own unmodified **Splatterhouse 3 (USA)** ROM. It must be exactly
2,097,152 bytes and match this SHA-256:

```text
8c7737912cf948a606a683e32f4b6a0a4303215cdc99b39fbc5976c196c45710
```

Name it exactly `Splatterhouse 3 (USA).md` and put it beside the executable.
Enable filename extensions in File Explorer to avoid a doubled extension.
Renaming a different ROM does not make it compatible. Check the hash with:

```powershell
Get-FileHash -LiteralPath '.\Splatterhouse 3 (USA).md' -Algorithm SHA256
```

### 4. Launch

Double-click `Splatterhouse3-Coop.exe`, or run:

```powershell
.\Splatterhouse3-Coop.exe
```

Press Enter at the title screen; player two joins automatically during gameplay.
Keep `engine\genesis_plus_gx_libretro.dll` in its folder beside the executable.
No shortcut is required or included. Saves are written to `coop-save.state`
beside the executable, so keep the project in a folder you can write to.
See the controls below, or the optional [BizHawk setup](bizhawk/README.md).

### Troubleshooting

- **Executable missing:** build first; GitHub's source ZIP contains no binaries.
- **Compiler or make not recognized:** check `-ToolchainBin` points to the
  folder containing all three compiler tools listed above.
- **ROM missing or differs from the supported version:** check its location,
  exact filename, size, and SHA-256. Extract compressed ROM files first.
- **Unable to load the experimental emulator core:** check that
  `engine\genesis_plus_gx_libretro.dll` exists and was built with the same
  64-bit toolchain as the launcher. Keep the generated folder layout intact.
- **Could not save game:** move the project to a writable folder.

## Play

### Launch and controls

After installation, run `Splatterhouse3-Coop.exe`.
Press Enter at the title screen. Co-op starts automatically during gameplay.

| Action | P1 | P2 | Xbox-style controller |
| --- | --- | --- | --- |
| Move | WASD | Arrow keys | D-pad / left stick |
| Attack | F | J | X |
| Transform | G | K | B |
| Jump | Space | L | A |
| Start / pause | Enter | Enter | Start |

F6 saves, F8 loads, F9 pauses the emulator, F5 resets to the title, Esc closes.
Save files include both players. Ordinary single-player emulator saves do not.
Two stacked rows use the original gold HUD frame and POW/LIFE meter style.
Blue P1 is on top, red P2 below. The fixed 320x254 output adds 30 pixels below
the original playfield for the second row. Lives are shared. Both Ricks must gather at the same door before leaving. During the door
entry sequence, left/right travel uses equal Y / different X; up/down travel
uses equal X / different Y. Both emerge separately.
The Rick nearest the exit leads the entry animation; the follower walks in
before the room changes.

For BizHawk, see [setup](bizhawk/README.md).

The previously frozen QuickSave1 now loads directly: the core repairs the
sound driver's lost bus request when it reaches the stalled wait loop. Your
original save file does not need conversion.

## Debug cheats

During gameplay, press **Genesis X** to kill the enemies in the room, including
the boss. Death animations and room/stage progression run normally. It triggers
once per press and is inactive on the map, in menus, and during door transitions.
In the current BizHawk bindings this is **V** (P1) or **U** (P2); keyboard **X**
is already Attack. In the standalone launcher, use **X** (P1), **O** (P2), or
the Xbox-style controller's **Y** button.

Press **Start** to open the native map, then **A+B+C together** to open cheats. On the default keyboard
bindings this is **Enter**, then **F+G+Space** (or **J+K+L** for P2).
Use Up/Down to select, C/Jump to apply, and Start to resume. On the stage row,
Left/Right chooses stage 1?6. One-shot actions take effect when you resume.

The menu includes invincibility, infinite power, infinite shared lives, frozen
timer, heal both, fill both power meters, damage either player, defeat both to
test respawns, stage selection and restart stage. Toggles affect both Ricks and
are included in saves. Resetting the game clears them. Damage tests turn off
invincibility so their result is visible.

## Agreed design

- Windows standalone launcher and BizHawk custom core.
- Two independently controlled Ricks; recolored player two.
- Separate health and transformation meters.
- Both players must reach the same exit before changing rooms.

## Verified and remaining work

Opening-room checks pass for independent movement, attacks by either Rick,
separate damage, P2 transformation, death/respawn, and save/load replay. The first
door blocks a lone player and carries both into the next room with separate
health and power. The BizHawk adapter passes headless boot and save replay.

Sprite fixes keep P2 animation uploads out of P1 VRAM, recolor static punch
frames as well as animated tiles, and restore the correct player context for
held weapons. Controlled fixtures verify pickup and visible attachment of all
three weapon types for either Rick. Debug tests cover the menu, all six stage
loads, cheats, damage and shared-life respawns.

This is not a completed campaign release. Later rooms, bosses, all door
orientations, extended simultaneous-power combat, extended weapon/grapple interactions, exhausted lives
and continues still need testing. P2 currently inherits P1's pose on room entry,
so retaining different transformation forms across a door needs further work.
Physical controllers and interactive BizHawk play have not been tested.

## Development

The original USA ROM remains unchanged. Supported SHA-256:
`8c7737912cf948a606a683e32f4b6a0a4303215cdc99b39fbc5976c196c45710`.

`build.ps1` builds the modified Genesis Plus GX core with a Windows x64 MinGW
toolchain. Pass `-ToolchainBin` to override the local default.
`python tools/probe.py` boots the ROM headlessly and records local snapshots.
Python tools require Pillow; disassembly tools additionally require Capstone.

Generate local test fixtures, then run the focused checks:

```powershell
python tools/prepare_fixtures.py
python tools/verify_coop.py
python tools/verify_attacks.py
python tools/verify_saved_monster.py
python tools/verify_transform.py
python tools/verify_power_duration.py
python tools/verify_mixed_forms.py
python tools/verify_death.py
python tools/verify_room_gate.py
python tools/verify_door_animation.py
python tools/verify_adapter.py
python tools/verify_sprites.py
python tools/verify_pickups.py
python tools/verify_debug.py
python tools/verify_debug_menu.py
python tools/verify_debug_respawn.py
```

The exit fixture deliberately clears enemies; it tests door coordination.
The attack checks separately defeat opening enemies through controller input.
See [research notes](RESEARCH.md) for hooks, architecture and test scope.

The engine was copied from `C:\TEMP2\aladdinGameCoop\engine`.
See `engine/LICENSE.txt` and `THIRD_PARTY_NOTICES.md` for upstream notices.
ROMs, local snapshots and generated diagnostics are not distribution assets.
