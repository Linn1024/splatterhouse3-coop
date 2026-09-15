# Splatterhouse 3 USA: initial co-op implementation

## Confirmed native paths

| Address | Role |
| --- | --- |
| `0032F0` / `0032F6` | Before/after player update in active gameplay |
| `00B660` | Native Rick update |
| `FF0021` / `FF001F` | P1 held / newly pressed buttons |
| `FF0022` / `FF0020` | P2 held / newly pressed buttons |
| `FF5520` | Rick's entry in the word-sized object tables |
| `FF5FA2` / `FF60A2` / `FF61A2` | Rick world X / animated Y / ground Y, 16.16 fixed point |
| `FF00A0` | Rick's action flags |
| `FF00B2` / `FF00BA` | Health / transformation power |
| `FF00B4` | Shared remaining lives |
| `FF00F0` / `FF00F8` | Camera X / Y |
| `011D0E` / `011F90` | Per-world-object update boundary |
| `022FFC` | Player/object contact routine |
| `02326E` / `02346E` | Door overlap test / successful scripted exit |
| `0079BC` | Native sprite-list construction |
| `007A02` / `007A74` | Before/after one sorted object's sprite construction |
| `00B51A` | Player animation tile upload |

## Architecture

The original ROM is unchanged. The CPU hook runs the native Rick update twice
with separate player context and input, then runs world objects once. Enemy AI
receives the nearest player's context, with grapple-owner preference. Native
contact checks run against each Rick. Animation scratch at `FF009A/FF009C` stays
shared within an object's update; capturing it as player state corrupts enemies.

Object storage is a set of parallel word/long arrays, not contiguous records.
In particular, treating every field after offset `A80` as a long array would
overwrite enemy fields. Only identified player fields are captured.

P1 retains native VRAM. P2 animation DMA at `B5D0` copies ROM bytes into a
private 96-tile buffer and skips the native transfer, so an emulated scanline
never sees P2 art in P1's live tiles. Native SAT builder hook `7B50` records
ownership per sprite; `7BDC` commits ownership with the sprite list. The mode-5
renderer selects private P2 rows below tile 96 and recolors both private rows
and static native frames above that boundary. Punches use both kinds. Clothing
inks 10?13 map to red palette entries; held item types 5?7 retain their colors.
Legacy virtual tile names are still accepted when loading earlier saves.

Each object's original AI/owner target is restored after both collision passes.
Held-item slot `D1` and action bit 5 override nearest-player selection, as does
the grapple owner. This keeps native held-frame mapping `B5D8` under its Rick's
context. Expanded sprite scanline limits follow the Aladdin approach.

P2 is inserted into the native sprite drawing order using ground Y. The shared
horizontal camera uses the midpoint; native screen-edge movement limits remain.
Doors require the partner within 24 world pixels horizontally and 16 vertically
of the same door. The native primary-slot scripted walk performs the transition;
P2 is recreated after loading with its previous health and power.

The save extension includes context-switch phases, CPU registers held between
passes, both graphics buffers, and pending transition metadata. The outer
Libretro adapter saves the last frame too. Native-only research snapshots do
not contain P2 and must be loaded with co-op disabled, then explicitly enabled.

## Scope of verification

Opening-room tests cover independent movement, enemy attacks, attacks by either
Rick, a P2 transformation, death/respawn, save replay and the first door. The door
fixture removes enemies to isolate the gate; separate combat tests defeat the
opening enemies through button input. Adapter checks are headless Libretro
checks, not a physical-controller or interactive BizHawk playthrough.

Later rooms, bosses, simultaneous transformations, extended weapon/grapple interactions,
all door orientations, exhausted lives/continues, and a full campaign remain
unverified. Room entry currently initializes P2's pose from P1, so independently
transformed players crossing a door need further work. No full-campaign claim.

## Sprite/debug regression checks

`verify_sprites.py` checks visible blue P1 / red P2 clothing across native punch
frames. `verify_pickups.py` injects native item records into the opening room,
then uses attack input to pick up types 5, 6 and 7. It checks the held flag,
attachment coordinates, and rendered pixel differences when the held object is
hidden. This is a controlled fixture, not a campaign weapon playthrough.

Debug flags, queued commands and packed menu state occupy unused player-snapshot
bytes (outside installed game RAM ranges). SAT ownership uses unused bytes too;
the existing state extension size remains unchanged. Normal reset clears these
fields. Debug tests cover all six stage loaders, toggles, menu save/load, damage,
defeat-both and native respawns with two shared lives consumed.

## Frozen quick-save recovery

The September 15 QuickSave1 has Z80 bus state 1 (running, no request) while the
main CPU is in the sound driver's BUSACK wait at `60C82`. Both previous builds
loop there indefinitely. Changing only the bus request makes the scene resume.
Co-op's longer frame permits an interrupt between the driver's request and poll;
the sound interrupt can release that request before the main driver resumes.

The co-op hook now reasserts BUSREQ at that exact wait PC only when the Z80 is
running without a request. It calls `gen_zbusreq_w`, preserving synchronization
and bus memory mapping. Other CPU locations and reset states are unaffected.
The original save loads without file conversion or changes to player progress.

`python tools/verify_frozen_save.py [path-to-BizHawk-save]` verifies that the
original archive resumes, both players respond to movement, a newly saved state
replays deterministically, and the source archive's SHA-256 remains unchanged.

## Mixed normal/powered sprite corruption

The native `B5C8` and `75D2` instructions write only D7's low word. During power
animations the upper word retains action bits (observed `01000180`, rather than
`00000180`, at the DMA call). The old 32-bit bounds check rejected those P2
transfers, allowing them to overwrite P1 VRAM. The hook now reads a 16-bit length
and preserves D7's upper word when returning the native low-word result of 1.

`verify_mixed_forms.py` keeps enemies alive but distant (native power activation
is disabled in a cleared room), records valid normal idle animation banks, and
checks that the idle partner's banks remain valid during either Rick's power
animation and attacks. Exhausting power tests native return-to-normal; mixed
states replay deterministically. It also exercises both transformations together.
The test fails on the preceding build for P2 transformation and passes with the
fix. Screenshots cover each mixed form and both powered Ricks.

## Door duet presentation

During an accepted room transition, the renderer now draws a second recolored
copy of the native scripted actor instead of suppressing P2 until room entry
finishes. On emergence, P2 leads by 32 world pixels for left/right travel;
up/down travel uses 24 pixels sideways and 20 along the door direction to keep silhouettes
separate. The entering P2 context starts at the same offset, avoiding a pop to
an unrelated position when control resumes. The native room sequence, fades,
map screen and gate conditions remain in control of the game.

SAT ownership value 2 selects recolored native art for the cinematic follower,
without sampling the previous room's private P2 tiles. The follower disappears
when the native actor is inactive, so it does not appear over the map/loading
screens. It uses existing serialized transition state; state size is unchanged.

`verify_door_animation.py` verifies exit and entry ownership, horizontal arrival
spacing and save replay during the cinematic. `verify_user_doors.py` additionally
checks real right/up exits with either Rick first; left/down native paths still
need playthrough coverage. Existing gate and separate health/power tests pass. Independent
transformation poses during room transitions remain a separate limitation.

## Premature return from power mode

Native power depletion at `B766` clamps the meter to `BD * 20`. Door recreation
previously copied P1's BD tier into P2 while restoring only P2's BA meter; a zero
tier erased the retained power on the first 60-tick depletion. The saved power
word now also packs P2's BC depletion timer and BD tier in its unused upper bytes.
Both are restored on entry. Before either Rick updates, a meter larger than its
tier's capacity raises that tier to the minimum valid value, repairing legacy
saves without extending a valid meter or lowering an already higher tier.

Co-op also bypasses the native room-clear power cancellation for an already
powered Rick (`B6B6`, `C8A4`). Death and empty-meter paths remain native. Starting
a new transformation in an already cleared room still follows native input rules.

`verify_power_duration.py` checks both players for four seconds with ordinary
and mismatched tier/meter combinations, natural depletion, independent partners,
return after exhaustion, and retaining power when the room clears mid-animation.
Door tests additionally assert that P2's tier and timer survive a room change.
The earlier 60-frame transformation check was too short to catch this failure;
it now runs for 240 frames.

## Door entry alignment revision

Outgoing native door commands 1..4 separate the actors along the travel axis:
32 pixels in X for left/right, or 40 pixels in Y for up/down. Entry
into the next room keeps the existing directional leading-step layout and final
spawn positions. No new transition state or save format is needed.

The door regression now inspects the native sprite builder's pieces to verify
matching Y and 32-pixel X spacing for rightward entry, plus the other three
directions using injected direction values. It also checks
that the next-room arrival stays 32 pixels apart and cinematic saves replay.

## QuickSave1 static attack-art corruption

The September 15 21:34 QuickSave1 reproduced a powered P2 attack assembled from
normal Rick tiles. In addition to the private animated banks, the game uses a
shared static-art region starting at tile 96. Loader table `7934` maps normal
art to ROM `AC000` (63 tiles) and powered art to `AC7E0` (72 tiles). The queued
CPU transfer at `786E` changes this shared region after transformations. A
mixed-form pair cannot both rely on its contents.

Sprite metadata now carries the visible frame's form as well as ownership.
Frames below 125 use normal art; powered frames start at 125. The renderer reads
these static tiles directly from the corresponding immutable ROM set for each
Rick. Dynamic/private tiles, backgrounds and item colors keep their existing
paths. This avoids both stale saves and subsequent shared loader changes.

Unused snapshot byte `306` marks the metadata revision. Older saves upgrade
builder/live flags from the saved actor frame IDs on restore, so the first
emulated frame uses the right art. The save extension size is unchanged.

`verify_quicksave_sprites.py` loads the original archive without modifying it,
replaces only the shared static VRAM region with normal, powered and blank data,
and checks that the rendered frame stays identical. It also checks combat replay
and the original file hash. The prior core fails this test; the updated core
passes. `diagnostics/quicksave1-sprites-fixed.png` records the corrected scene.

## Door orientation, saved monster and native pause map

Door presentation uses travel direction: left/right travel has equal Y with
a 32-pixel X separation; up/down travel has equal X with a 40-pixel Y
separation. This corrects the previously reversed axes. Arrival offsets are
unchanged. The regression checks both axis arrangements in the native sprite
builder; left/up/down checks use injected direction values rather than full
room traversals.

The 21:59 QuickSave1 contains a regular purple monster (type 46 hex) executing
its ordinary AI loop while retaining collision category 4. Native contacts
explicitly skip that category. On re-entering that monster's ordinary AI at
`13BF4`, co-op restores category 3 and removes its stale byte from the exclusion
list at `226E`. Scripted states and held objects do not take this repair path.
The original archive remains unchanged. `verify_saved_monster.py` demonstrates
that either Rick can kill it with ordinary attack input (80/100 tested frames),
without modifying its HP or applying a kill cheat.

Start now reaches the native map branch at `32A2` during active gameplay, even
with enemies present. The frontend no longer freezes emulation on Start.
Once map phase 3 is ready, ABC opens the cheat overlay and freezes only that
overlay. Start resumes through the native map exit; Back to Map closes cheats.
The native mode and packed overlay state remain part of saves. Legacy native
pause flags can still be resumed normally. Both frontends use the same logic.
Map checks verify frozen gameplay status, cheat toggles, saved overlays and
return to gameplay; the native map was visually inspected.

## QuickSave1: both real door orientations and both leaders

`verify_user_doors.py` walks from the user's two-door QuickSave1 using controller
inputs only. It tests the right-hand exit and upper exit with blue arriving first
and red arriving first, keeping the partner away for 400 frames before joining.
The source archive hash is checked afterward; the test never overwrites it.

The native contact that finally opens a door can belong to the trailing Rick.
Entry leadership is therefore selected from the actors' positions along the
travel direction, rather than the contact owner. Reserved snapshot byte `307`
stores blue/red leadership (1/2, zero for old cinematic saves).

The native script now moves the follower; the separately rendered actor is the
leader, 32 pixels ahead for left/right travel or 40 for up/down. The leader hides
at the doorway, and room loading waits for the follower. This fixes near-instant
side-door transitions and the upper-door actor floating above the doorway.
Native doorway alignment is copied when red triggers the contact, and a second
contact cannot restart the same transition. Rendering swaps entry colors when
blue leads; subsequent emergence and final spawn placement retain their existing
layout. No save format size change is required.

All four real routes verify entry order/spacing, separate disappearance, next-room
arrival, unchanged health/power, both players' controls, and deterministic replay
from an in-progress entry save. Upward entry takes 96 frames in this fixture;
rightward entry takes 23. Left/down rendering still has synthetic legacy-save
coverage, not a claim of full native traversal coverage.

## Map graphics preservation and non-gameplay rendering (QuickSaves 1-5)

The native map overwrites enemy VRAM. Originally it was only available after
clearing the room; allowing it during fights exposed that destructive reuse.
At `32B6`, co-op now snapshots VRAM into the unused upper 32 KiB of each private
player context, plus SAT metadata at snapshot offset `7000`. The first resumed
player update restores that memory and invalidates the pattern cache. Snapshot
byte `308` marks the backup; saving/loading while paused preserves it without
changing the save extension size.

Old QuickSaves 2 and 3 already contain map fill (`E400` repeatedly at VRAM `6000`)
in enemy graphics. For that signature only, the first gameplay update rebuilds
the room's native enemy graphics lists from ROM (`5610E` scripts, `55B1C` list
pointers, `120000` compressed art). It does not reset enemy health, AI, palette,
room progress or players. Snapshot byte `309` prevents repeated repair. Old
saves captured on the map defer recovery until gameplay resumes.

Co-op sprite overrides are restricted to gameplay/door phases (mode 4, phases
8-10). Map markers and story sprites use native rendering. High tile numbers
are interpreted as old private Rick aliases only for explicitly owned pieces;
otherwise door/scenery graphics remain native. Per-piece Rick metadata is only
assigned to Rick's palette and tile range, including Rick arms in composite
grab frames. P2 insertion respects the native foreground prefix at `FF0192`.
The co-op HUD is hidden on story/level-completion screens, and stage setup at
`3002` resets stale player contexts before a fresh co-op spawn.

Stage 6 in RAM is the native Stage X / Strange Zone bonus area. Native stages
above 4 retain ordinary pause, since they have no native floor map; the debug
menu still opens through A+B+C while paused.

`verify_reported_sprites.py` checks the five archived reported saves: exact
monster/boss tile recovery against an independent native-format decoder,
map preservation and paused-state replay, 45 frames of map/story pixels against
the native renderer, story-to-gameplay controls, Stage X pause, and unowned high
tile handling. `verify_user_doors.py` covers both real door axes and either
leader. Original save archives are unchanged. Mixed-form sprites, pickups,
door gating, debug menu and fixed display geometry also pass their regressions.

## QuickSave6: red stuck on an old room row

QuickSave6 has red at Y=664 (`0298`) while the room camera starts at Y=1024
and blue stands at Y=1197. Native vertical clamps at `CEE0` / `D08C` keep the
actor's existing high coordinate byte and clamp its low byte to `98`. That
perpetuates the invalid row: Up/Down change fractional ground Y but animated Y
is reset to 664 every frame. The screen's 512-pixel sprite-coordinate wrapping
makes red appear in the room despite the wrong world coordinate.

Before P2's ordinary update, when gameplay is active and no door script is
running, mismatched ground-row bits are rebased onto the current room's camera
row. Animated Y and ground Y receive the same 32-bit offset, preserving jump
height and fractional movement. Screen Y is refreshed. X, animation state,
health and power are not reset. This also recovers the old save automatically;
its file remains untouched. The exact earlier transition that produced the
stale row has not been identified from this snapshot alone.

`verify_y6.py` checks recovery by +512 pixels, Up/Down movement, blue and meters
unchanged, retained powered form, deterministic replay and fractional/vertical
pose preservation. The door-route, reported-sprite, power-duration and basic
co-op regressions still pass.

## Third-floor initialization boundary

The user's clarification places QuickSave6's onset at the beginning of floor 3.
A direct native floor-three room-load fixture reproduced the initialization gap:
mode 4 phase 8 replaced the primary actor while `sh_ready` and P2's previous
context survived. The earlier reset at `3002` only covered stage setup, not the
common room-loading boundary. This can expose the stale actor during native
entry and leave its coordinates to be repaired later by the Y guard.

The hook at `3112` now invalidates co-op readiness and in-flight actor/render
context at every native room load. The normal first playable update initializes
P2 from the newly loaded primary actor. Accepted door transitions retain their
saved P2 health/power for the existing handoff. Old QuickSave6 still uses the
coordinate recovery; fresh floor loading no longer depends on it.

`verify_floor_start.py` deliberately enters the native third-floor loader with
QuickSave6's stale P2 context, bypassing the earlier stage-setup hook. It checks
that red initializes exactly 32 pixels beside blue at matching Y and can move
vertically. This is a native-loader regression, not a full second-floor boss to
third-floor playthrough. QS6 recovery, all four real door routes, door health/
power handoff, and reported sprite/map/story save regressions also pass.

## Genesis X: kill room cheat

The shared frontend input handler sends debug command 10 on a rising edge of
RetroPad L (Genesis X), only during unpaused gameplay. Existing Start/map/menu
behavior takes precedence. The current BizHawk bindings map that control to V
for P1 and U for P2; keyboard X remains the user's Attack binding. Standalone
already maps Genesis X to keyboard X/O and Xbox Y. No bindings were changed.

At the next safe player-update boundary, command 10 gives active native combat
slots 19-22 a fatal native reaction (12104 table, reaction 3/action 2) and zero
health. It clears affected player grapple references. Door/item slots and both
players are retained. Native enemy death scripts handle removal, room clear and
boss/stage completion; the cheat does not delete the room's object table.

`verify_kill_room.py` checks both players' triggers against the reported regular
monster and boss saves, held-button behavior, save replay, boss progression,
player health, map rejection and empty-room door preservation. Debug-menu and
ordinary co-op movement/combat regressions also pass.

## Boss palette and stationary boss follow-up

The newer QS1/QS2 have palette 8 (map colors) in the active fourth palette bank
at `4D6E`, while its native target at `4DEE` remains palette 79, the proper boss
colors. VRAM preservation alone did not preserve these active palette buffers.
Map backup version 2 now also stores both 128-byte active/target palette blocks
at private snapshot `70A0` and restores them on resume, requesting a native
CRAM upload. Version 1 paused saves skip the absent palette backup safely.

During ordinary boss gameplay without a fade or door script, an exact match of
the active boss bank to map palette 8, with a different native target bank,
recovers from that target. Other palettes and active color effects are retained.
This repairs the reported saves without changing their files.

QS2 has blue grappling boss type 33, with player grapple target `0026` (slot 19).
The boss stays still while held in the original game as well. Repeated Attack
releases it after 56 frames, and its position subsequently changes. No boss AI
movement change was made. `verify_boss_review.py` compares the native and co-op
held/release traces, then checks both move after release; later movement may
differ because co-op can select a different player target. It also checks both
reported palettes, repeated map transitions, paused-state replay and old backup
compatibility. Earlier map/sprite and debug-menu regressions pass.

## Regular monster palette after map (new QS3)

QS3 is paused on the third-floor map with live types 4E/4A. The map has
palette 8 in active bank 3, while the native target is palette 195. The
installed core differed from the staged palette-preserving build. The same
shared-bank overwrite affects ordinary monsters, not just bosses.

Map entry preserves all 256 bytes of active/target palette RAM alongside
VRAM and sprite ownership. Restoration now runs at native map-exit RTS
743A, after gameplay mode is restored; waiting for player update 32F0 left
one frame with map colors in gameplay mode. The latter hook remains a
fallback for older snapshots. Exact stale-map palette recovery also covers
ordinary enemies when an older snapshot lacks the palette backup.

`verify_map_palettes.py` uses QS3 as a reproduction, then runs a native room
reload to obtain clean room palettes and live enemies. Three input-driven
map cycles preserve every active/target bank on every returned gameplay
frame, including replay from a save taken inside each map. Boss review,
reported sprite/map/story regressions and debug-menu checks pass. Source
save files are unchanged; no save files are patched.


## Stacked native-style HUD and taller frontend

Both frontends now compose a fixed 320x254 surface: the previous normalized
320x224 game frame plus 30 rows beneath it. Aspect ratio is (4/3)*224/254,
so the game image keeps its previous proportions. Serialized video remains
the untouched native frame and the state format/size is unchanged.

The HUD reuses the live native footer (rows 194-223), including its frame,
lettering and meter surrounds. It duplicates that footer for P2 and draws
independent meters in the original interiors (POW x48..127, LIFE x168..231,
y205..210 in native 256-dot coordinates), using the native meter gradients.
Blue P1/red P2 labels distinguish the rows. The added strip stays black when
the co-op HUD is inactive. No ROM artwork is bundled as an extracted asset.

Build and adapter/display/map/debug regressions pass. `verify_stacked_hud.py`
checks independent player damage, power refill, map display and saved-frame
replay. The fixed-size check includes both native widths, stage changes and
reset, and checks the new aspect ratio.


## Gut blast and final door-entry color

The powered Back/Forward/Back+Attack move switches native static art from
loader table 7934 entry 1 (AC7E0, 72 tiles) to entry 2 (AD0E0, 109 tiles).
The co-op static-art override previously had only normal and powered sheets,
so it displayed powered body pieces in place of the extended guts, and did
not tag the tail beyond tile 167. SAT bit 16 now selects the third sheet for
native gut-blast poses 107-109 and includes its full extent through tile 204.
Each sprite records its own sheet/ownership, preserving the other player's
art even when native global VRAM changes. The save-state layout is unchanged.

Door entry now retains a latch at private metadata 30A until the common room
loader at 3112. Native exit command C2 becomes zero before the last visible
frame; using that command alone prematurely dropped the blue-leader/red-
follower color swap. Older in-progress saves infer the latch from a live
entry command. Arrival clears it before the emergence animation.

`verify_gut_blast.py` finishes transformation, performs the actual input
sequence for each player and checks 8,320 rendered static rows per player
against the original gut sheet, including flipped/recolored rows, its long
tail and mid-attack save replay. `verify_user_doors.py` now rejects a follower
changing color after the leader disappears; right/up doors with either leader
pass. Legacy door-animation, mixed-form and reported sprite/map/story checks
also pass. Builds are staged for the launcher; user saves are unchanged.


## Left-facing gut blast and door fade follow-up (QS2/QS3)

The first gut-blast fix covered right-facing poses 107-109 only. Native
left-facing poses BA-BC are separate mappings, not the same pose IDs with a
flip bit. Both sets now select the 109-tile AD0E0 sheet. The gut-blast test
explicitly sets facing and performs the command in both directions for each
Rick (four cases), checking 8,320 native static rows per case and save replay.

QS3 captures native fade mode 5 with saved return mode 4. The previous entry
latch preserved SAT ownership, but the renderer rejected it because mode 5
was outside sh_scene(). Sprite ownership and private art now remain active
through this door fade when sh_transition is set and the saved return mode
is gameplay. Map/story rendering remains unchanged. The door regression now
requires fade frames and checks live SAT ownership throughout them.

`verify_reported_left_door.py` replays both newly archived saves, verifies the
left sheet and red follower's fade ownership, and checks deterministic replay.
Both reports were visually inspected after the fixes. Mixed forms, full
right/up door sequences with either leader, and reported map/monster/story
regressions pass. The installed core matched the previous build at diagnosis;
these were missing cases in that fix, not a failure to install it. User save
files are unchanged. Older QS2 contains an already-built sprite table; its
normal next SAT rebuild picks up the corrected left-facing metadata.
