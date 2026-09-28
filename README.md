# Splatterhouse 3 Co-op

An experimental two-player co-op version of Splatterhouse 3 for Windows. Play
as two independently controlled Ricks with separate health and power meters,
shared lives, and coordinated room exits. The original game ROM stays unchanged.

## Download and install

**[Download the Windows installer](https://github.com/Linn1024/splatterhouse3-coop/releases/download/v0.1.0/Splatterhouse3-Coop-0.1.0-windows-x64-setup.exe)**

1. Run the installer on **Windows 10 or 11, 64-bit (x64)**.
2. When prompted, select your own **unmodified Splatterhouse 3 (USA) ROM**.
   Setup verifies it and copies it into the game folder. You can also skip
   this step and add the ROM later.
3. Finish setup and open **Splatterhouse 3 Co-op** from the Start menu.
4. Press **Enter** at the title screen. Co-op starts automatically in gameplay.

No compiler, Python, separate emulator, or administrator access is required.
The installer creates Start-menu shortcuts and offers an optional desktop shortcut.
**The game ROM is not included.**

Prefer no installer? Download the
[portable ZIP](https://github.com/Linn1024/splatterhouse3-coop/releases/download/v0.1.0/Splatterhouse3-Coop-0.1.0-windows-x64-portable.zip),
extract the entire archive to a writable folder, add your ROM as described below,
and run `Splatterhouse3-Coop.exe`. Keep the `engine` folder beside the executable.
GitHub's **Code > Download ZIP** contains source code; use the release downloads
above to play without building.

### Supported ROM

The supported ROM is exactly **2,097,152 bytes**, with SHA-256:

```text
8c7737912cf948a606a683e32f4b6a0a4303215cdc99b39fbc5976c196c45710
```

The installer accepts any filename and renames its imported copy automatically.
To add it manually, open **Game folder** in the Start-menu group and place the
ROM beside `Splatterhouse3-Coop.exe`, named exactly `Splatterhouse 3 (USA).md`.
For the portable version, use the folder you extracted. Extract compressed ROMs
first; renaming another version will not make it compatible.

To verify a file yourself, open PowerShell in its folder and run:

```powershell
Get-FileHash -LiteralPath '.\Splatterhouse 3 (USA).md' -Algorithm SHA256
```

## Controls

Both players can use one keyboard. Two Xbox-compatible controllers are also
supported; controller hardware has not yet been tested extensively.

| Action | Player 1 | Player 2 | Xbox-style controller |
| --- | --- | --- | --- |
| Move | WASD | Arrow keys | D-pad / left stick |
| Attack | F | J | X |
| Transform | G | K | B |
| Jump | Space | L | A |
| Start / map | Enter (shared) | Enter (shared) | Start |

**F6** saves, **F8** loads, **F9** pauses the emulator, **F5** resets to the
title screen, and **Esc** closes the game.

Blue P1 appears on the upper HUD row and red P2 on the lower row. Lives are
shared. Both players must reach the same door to leave a room.

### Saves, updates, and uninstalling

The standalone game uses one save slot: `coop-save.state` in the game folder.
F6 replaces that slot. Saves include both players and require this custom core;
ordinary single-player emulator saves are not interchangeable.

Close the game before installing an update into the same folder. Back up your
save before updating; compatibility across prototype versions is not guaranteed.
Uninstall through Windows Settings > Apps. Uninstalling preserves your imported
ROM and save files; you can remove those remaining files yourself if desired.

### Optional cheats

During gameplay, **X** (P1), **O** (P2), or controller **Y** defeats the enemies
in the current room, including bosses.

Press **Enter / Start** to open the map, then **Attack + Transform + Jump**
together (**F + G + Space**, or **J + K + L**) to open the cheat menu.
Use Up/Down to select, Jump to apply, and Start to resume. Left/Right changes
the stage selection from 1 to 6. One-shot actions take effect when play resumes.
The menu includes invincibility, infinite power and shared lives, frozen time,
healing, damage tests, stage selection, and stage restart. Toggles affect both
players, persist in saves, and clear on reset.

## Troubleshooting

- **ROM missing or unsupported:** check the filename, location, and checksum
  above. In File Explorer, show filename extensions to avoid a doubled extension.
- **Emulator core cannot load:** reinstall, or extract the complete portable ZIP.
  The launcher needs `engine/genesis_plus_gx_libretro.dll` beside it in that layout.
- **Saving fails:** use the installer's default folder or a writable folder for
  the portable version.
- **An old save will not load:** use the same project version that created it,
  or start a new game. Keep backups before trying a different version.

Report problems in [GitHub Issues](https://github.com/Linn1024/splatterhouse3-coop/issues)
with the release version, Windows version, and steps to reproduce them. Do not
attach game ROMs.

## Prototype status

This is an early prototype, not a fully tested campaign release. Opening-room
checks cover movement, attacks, independent damage, transformation, respawn,
saving/loading, and coordinated room exits. Later rooms and bosses, prolonged
combat, weapon/grapple interactions, exhausted lives, and continues need more
testing. P2 currently inherits P1's pose on room entry, so retaining different
transformation forms across doors needs further work.

The optional [BizHawk adapter](bizhawk/README.md) has passed headless checks;
interactive BizHawk play remains unverified.

## Source and development

See [DEVELOPING.md](DEVELOPING.md) to build the game or installer and run checks.
[RESEARCH.md](RESEARCH.md) describes the emulator hooks and known limitations.
Binary packages include their matching source in `source.zip`, with the revision
recorded in `SOURCE.txt`.

The modified Genesis Plus GX engine retains its
[license and component notices](engine/LICENSE.txt), including noncommercial
redistribution conditions. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
for attribution. Game ROMs are not distributed with this project.
