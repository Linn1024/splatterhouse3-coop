"""Map palette preservation during combat, including a save inside the map."""
from probe import *
import struct, zipfile

source = ROOT / 'diagnostics/map-palette-qs3.State'
original = source.read_bytes()
with zipfile.ZipFile(source) as z:
    raw = z.read('Core.bin')
seed = raw[4:4+int.from_bytes(raw[:4], 'little')]
p = Probe(ROOT / 'bizhawk/splatterhouse_coop_libretro.dll')
c = C.CDLL(str(ROOT / 'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype = C.c_void_p
p.ram_ptr = c.retro_get_memory_data(2)
p.restore(seed)
p.run(3)
assert p.ram()[1:3] == bytes([11, 3])
expected_enemy = p.ram()[0x4dee:0x4e0e]
p.run(1, (3,))
p.run(150)
assert p.ram()[0x4d6e:0x4d8e] == expected_enemy
p.screenshot(ROOT / 'diagnostics/map-palette-qs3-resumed.png')
print('PASS older map-open QS3 resumes with native monster palette')

# A native room reload supplies clean graphics/palettes and live enemies.
# Subsequent cycles use controller input only, without save-specific repair.
p.write(2, bytes([8]))
p.run(180)
assert p.ram()[1:3] == bytes([4, 10])
assert any(int.from_bytes(p.ram()[0x551e+s*2:0x5520+s*2], 'big') & 0x8000
           for s in range(19, 23))
for cycle in range(3):
    before = p.ram()[0x4d0e:0x4e0e]
    p.run(1, (3,))
    p.run(90)
    assert p.ram()[1:3] == bytes([11, 3])
    paused = p.save()
    header = struct.unpack('<8I', paused[:32])
    meta = 32 + header[2] + 144
    assert paused[meta+0x308] == 2
    assert paused[meta+0x70a0:meta+0x71a0] == before
    assert p.ram()[0x4d6e:0x4d8e] != before[96:128], 'Map must exercise palette replacement'
    for replay in range(2):
        if replay:
            p.restore(paused)
        p.run(1, (3,))
        checked = 0
        for frame in range(150):
            p.run(1)
            if p.ram()[1:3] == bytes([4, 10]):
                assert p.ram()[0x4d0e:0x4e0e] == before, (cycle, replay, frame)
                checked += 1
        assert checked > 60
    print('PASS cycle', cycle+1, 'all active/target banks preserved on every gameplay frame, including paused save replay')
assert source.read_bytes() == original
print('PASS source save unchanged')
