# Splatterhouse 3 Co-op

Playable prototype using the emulator-assisted approach from Aladdin Co-op.

## Play

Run `Splatterhouse3-Coop.exe` or `Play Splatterhouse 3 Co-op.lnk`.
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
