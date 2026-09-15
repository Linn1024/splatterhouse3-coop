"""Regressions for the user's map, monster, boss, story and bonus saves."""
from native_sprite_art import *
import zipfile
sources=[ROOT/f'diagnostics/reported-sprites-save{i}.State' for i in range(1,6)]
hashes=[hashlib.sha256(s.read_bytes()).hexdigest() for s in sources]
seeds=[]
for source in sources:
 with zipfile.ZipFile(source) as z:w=z.read('Core.bin')
 seeds.append(w[4:4+int.from_bytes(w[:4],'little')])
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
c.al_probe_vram.restype=C.POINTER(C.c_ubyte)
def vram():
 raw=bytes(c.al_probe_vram()[:65536]);v=bytearray(raw);v[::2]=raw[1::2];v[1::2]=raw[::2];return bytes(v)
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
def restore(seed):p.restore(seed);p.buttons[1]=set()
def framehash():return hashlib.sha256(p.frame[0]).hexdigest()
for slot in (2,3):
 restore(seeds[slot-1]);before=vram();r=p.ram();room=int.from_bytes(r[0x184:0x188],'big');stage=r[0x166]
 a=u32(u32(0x5610e+stage*4)+rom[room]*4);assert u16(a)==0xfffe;a+=2;expected=[]
 while u16(a)!=0xfffd:
  if u16(a)!=65535:expected+=gfx(u16(a))
  a+=6
 p.run(3);after=vram();assert after!=before
 for dst,data in expected:assert after[dst:dst+len(data)]==data,(slot,hex(dst),'native art mismatch')
 repaired=p.save();p.run(1,(3,));p.run(90);assert p.ram()[1:3]==bytes([11,3])
 paused=p.save();p.screenshot(ROOT/f'diagnostics/fixed-save{slot}-map.png')
 def finish_map():
  p.run(1,(3,));p.run(150);assert p.ram()[1]==4
  for dst,data in expected:assert vram()[dst:dst+len(data)]==data,(slot,hex(dst),'map damaged enemy art')
  return status(),framehash(),hashlib.sha256(vram()).hexdigest()
 first=finish_map();restore(paused);assert finish_map()==first
 p.screenshot(ROOT/f'diagnostics/fixed-save{slot}-resumed.png')
 print('PASS save',slot,'native monster/boss art restored; pause/map, paused save replay preserve art')
# Map UI and story screen should be pixel-identical to the native renderer.
for slot in (1,4):
 sequences=[]
 for enabled in (True,False):
  restore(seeds[slot-1])
  if not enabled:c.sh_coop_enable(0)
  frames=[]
  for _ in range(45):
   p.run(1)
   # Map deliberately keeps the co-op HUD in the bottom 30 rows.
   height=194 if slot==1 else 224
   frames.append(hashlib.sha256(p.frame[0][:p.frame[3]*height]).hexdigest())
  sequences.append(frames)
 assert sequences[0]==sequences[1],(slot,'non-gameplay overlay differs from native')
 print('PASS save',slot,'map marker/story screen matches native renderer over 45 frames')
restore(seeds[4]);p.run(2);assert p.ram()[0x166]==6
p.run(1,(3,));p.run(5);assert p.ram()[1]==4 and c.sh_coop_paused(),'Stage X should use native pause, not a nonexistent map'
p.run(1,(3,));p.run(5);assert not c.sh_coop_paused()
print('PASS save 5: native Stage X pause/resume, no nonexistent map')
restore(seeds[3])
for t in range(2400):
 p.run(1,(3,) if t%90==0 else ())
 if p.ram()[1]==4 and p.ram()[2]==10 and status()[0]:break
else:raise AssertionError('Story did not advance to the next playable stage')
before=status();p.buttons[1]={6};p.run(12,(7,));after=status()
assert after[2]>before[2] and after[4]<before[4],('Stale co-op context after story',before,after)
print('PASS save 4 story advances to a fresh playable co-op stage with both controls')
# Unowned high VRAM tiles belong to doors/scenery, not legacy red-Rick aliases.
c.sh_sprite_row.argtypes=[C.c_uint,C.c_uint,C.c_uint];c.sh_sprite_row.restype=C.c_void_p
assert not c.sh_sprite_row(0x7a0,0,0)
assert c.sh_sprite_row(0x27a0,0,1)
print('PASS native high sprite tiles bypass the legacy Rick alias path')
assert [hashlib.sha256(s.read_bytes()).hexdigest() for s in sources]==hashes
print('PASS all archived source saves unchanged')
