# Building and testing

Shared workflow: [common co-op development guide](../coop-guides/COOP-DEVELOPMENT.md).

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
python tools/verify_appearance.py
python tools/verify_appearance_title.py
python tools/verify_preview_art.py
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

For the reported appearance issue, run `python tools/verify_appearance_replay.py`
with the archived `diagnostics/appearance-reported-qs1.State`, or pass the path
to a matching BizHawk state as its argument. It checks QS1 movement/attack
poses and a cold start without changing the supplied save.

`python tools/verify_appearance_transitions.py <state-path>` checks that save's
actual selected colors through three map cycles and colored door entry/exit
cinematics using the door fixture. `powershell -NoProfile -ExecutionPolicy Bypass
-File tools/verify_bizhawk_launcher.ps1` checks replacing an in-use engine
without closing its running instance; it mocks process launch in a temporary
diagnostic installation.

`python tools/verify_appearance_jumps.py <state-path>` checks the reported
entrance and normal/powered jump and air-attack poses in both directions.
Its default fixture is `diagnostics/appearance-reported-qs1-v3.State`.

For the archived crowded-scene fixture `diagnostics/performance-qs1.State`,
`python tools/benchmark_qs1.py <adapter-directory> <output-json>` records 900
frames each of idle/combat input, host costs, native update cadence and video
hashes. Append `native` to run the co-op-disabled control. The archived
before/after/repeat outputs are checked by `python tools/verify_performance.py`;
its assertions use update counts and hashes rather than host timing thresholds.

### Portable standalone startup and settings

coop_desktop.h implements the shared ROM picker, Unicode remembered ROM path,
per-user settings, screen-size/fullscreen menu and settings-folder shortcut.
The original ROM bytes are validated before remembering a location; cancel
exits normally and a missing/changed file returns to the picker. Engine loading
uses the executable's absolute directory, while normal saves/settings use the
per-user Linn1024Coop folder. Existing standalone saves/controls migrate without
overwriting files already there. Test mode keeps isolated legacy diagnostics;
--rom and --settings-dir allow automated checks without changing user settings.
Keyboard/XInput configuration is available from Settings and F2. The source
copy of coop_desktop.h matches coop-releases/desktop/coop_desktop.h. Build
scripts link shell32 and comdlg32. verify_desktop.py in that repository drives
real picker input, external Unicode paths, relaunch, controls, screen resizing,
fullscreen and cancellation for all four games using isolated test folders.


## Standalone controls and ten save slots (2026-10-28)

The shared launcher controls use separate aligned Action/Keyboard/Controller
columns and a Clear button beside every binding. Controller means XInput
hardware; clearing a directional controller binding also disables that stick
direction. Clearing or remapping a save shortcut is persisted and overrides
the previous hardcoded host key. Defaults: F5 quicksave, F8 quickload, F6 next
slot, F7 previous slot. The Saves menu and controls window provide clickable
actions. Ten slots are numbered 1-10 with wrapping and a persisted selection.
Indiana slot 10 retains the legacy slot-0 file path for compatibility.
Aladdin saves include both the emulator state and its separate co-op extension,
plus the host pause/debug menu. Splatterhouse retains old slot-1 save loading.
Actual UI checks cover clearing/persistence, shortcut rebinding, every slot,
wraparound, independent writes, load/resave, and malformed-save rejection.
