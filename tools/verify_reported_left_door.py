"""September 16 QS2 left gut blast and QS3 red follower during fade."""
from probe import *

p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
for slot in (2,3):
 source=ROOT/f'diagnostics/left-door-qs{slot}.bin';seed=source.read_bytes()
 def replay():
  p.restore(seed);p.buttons[1]=set();frames=[];checked=0
  for t in range(120):
   p.run(1);r=p.ram();q=c.sh_probe_player(0)
   if slot==2 and t>=2 and int.from_bytes(bytes(c.sh_probe_player(1)[0x57a0:0x57a2]),'big') in (0xba,0xbb,0xbc):
    assert any(q[0x550+i]&16 for i in range(80)), 'Left guts sheet not selected'
    checked+=1
   if slot==3 and r[1]==5 and r[3]==4:
    red=[i for i in range(80) if q[0x550+i]&3]
    assert red, 'Missing red follower in saved SAT'
    assert all(c.sh_sprite_flags(i,0x2000)&3 for i in red), 'Fade discarded red ownership'
    checked+=1
   if t==2:p.screenshot(ROOT/f'diagnostics/left-door-fixed{slot}.png')
   frames.append(hashlib.sha256(p.frame[0]).hexdigest())
  assert checked>0
  return frames
 first=replay();assert replay()==first
 assert source.read_bytes()==seed
 print('PASS reported QS'+str(slot),'live rendering and deterministic replay; archived save unchanged')
