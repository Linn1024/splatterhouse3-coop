# Building and testing

Players can use the [installer or portable download](README.md#download-and-install).
These instructions are for building from source on 64-bit Windows.

## Build the game

1. Clone the repository, or download and extract its source ZIP:

   ```powershell
   git clone https://github.com/Linn1024/splatterhouse3-coop.git
   cd splatterhouse3-coop
   ```

2. Install a Windows x86_64 MinGW-w64 GCC toolchain, such as a Win64 package
   from [WinLibs](https://winlibs.com/). Its `bin` folder must contain `gcc.exe`,
   `g++.exe`, and `mingw32-make.exe`.
3. In PowerShell in the project folder, run the command below. Replace the
   example compiler path with your toolchain's `bin` directory:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -ToolchainBin 'C:\Tools\mingw64\bin'
   ```

   You can omit `-ToolchainBin` if the compiler tools are already on `PATH`.
   The execution-policy option applies only to that PowerShell process.

The build produces `Splatterhouse3-Coop.exe`,
`engine/genesis_plus_gx_libretro.dll`, and the two DLLs in `bizhawk/`.
Add the [supported ROM](README.md#supported-rom) and run the executable.
Python and BizHawk are not required for the standalone build.

## Build release packages

Install [Inno Setup 7](https://jrsoftware.org/isinfo.php) and Git. Commit the
source you intend to release. The packaging script exports a Git revision into
a fresh build folder, builds it, and packages only explicitly selected files.
Uncommitted files, ROMs, saves, and diagnostics are not copied into packages.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\installer\build-release.ps1 -Version 0.1.0 -SourceRef HEAD -ToolchainBin 'C:\Tools\mingw64\bin' -IsccPath 'C:\Tools\Inno Setup 7\ISCC.exe'
```

Both paths are examples; use your actual compiler locations. `ISCC.exe` may
also be on `PATH`. The script refuses to overwrite an existing version folder.
Move that folder elsewhere before rebuilding the same version.

`dist/<version>/` contains the setup EXE, portable ZIP, complete matching source
ZIP, and `SHA256SUMS.txt`. Publish those four files together. Both binary packages
also contain `source.zip`, the engine license, attribution, and a `SOURCE.txt`
record of the source revision. Publish from a commit or tag for traceability.

The installer uses a per-user writable application folder, validates an optional
ROM import by SHA-256, creates shortcuts, and preserves ROMs and saves during
uninstall. It supports `/ROMFILE="<absolute path>"` for automated testing.
With `/VERYSILENT /SUPPRESSMSGBOXES /NORESTART`, an invalid supplied ROM aborts
installation; an omitted ROM is allowed so it can be added later.

Before publishing, test a clean install with and without a ROM, rejection of an
invalid ROM, launcher boot, reinstall into the same folder, and uninstall with
save preservation. Check that the source archive and binary payload contain no
ROMs, personal profiles, or diagnostic captures. Installer creation alone does
not sign the executable.

Run the automated installation checks in a Windows account without an existing
installed copy. Replace the ROM path with your own file:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\installer\test-installer.ps1 -SetupPath .\dist\0.1.0\Splatterhouse3-Coop-0.1.0-windows-x64-setup.exe -RomPath 'C:\Games\ROMs\Splatterhouse 3 (USA).md'
```

The test installs and uninstalls temporary copies, verifies ROM rejection,
shortcuts, boot, and preservation of a sentinel save file. Test logs remain
under `dist/installer-test-*`; they are excluded from release packages.

## Gameplay checks

Install 64-bit Python and Pillow, then add your supported ROM beside the launcher:

```powershell
python -m pip install Pillow
python tools/prepare_fixtures.py
python tools/verify_coop.py
python tools/verify_attacks.py
python tools/verify_transform.py
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

Fixtures and captures are generated under `diagnostics/`. The exit fixture
deliberately clears enemies to isolate door coordination; combat checks are
separate. Some additional regression scripts require specific save-state
fixtures that are not distributed. Disassembly tools additionally use Capstone.
See [RESEARCH.md](RESEARCH.md) for implementation details and test boundaries.
