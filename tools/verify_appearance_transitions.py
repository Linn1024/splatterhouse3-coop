"""Current reported QS1 colors through map cycles and both door phases."""
from probe import *
import zipfile

source=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'diagnostics/appearance-reported-qs1-v2.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
c.al_probe_vram.restype=C.POINTER(C.c_ubyte)
c.sh_probe_vdp_regs.restype=C.POINTER(C.c_ubyte)
p.restore(d[4:4+int.from_bytes(d[:4],'little')])
appearance=c.sh_coop_appearance_get();checks=0;visible=0

def mask_check(label):
 global checks,visible
 seed=p.save();without=appearance&~0xff00
 for _ in range(2):
  p.restore(seed);c.sh_coop_appearance_set(without);p.run(1)
 native=p.frame[0]
 p.restore(seed);c.sh_coop_appearance_set(appearance);p.run(1)
 frame,w,h,pitch=p.frame
 pixels=[(x,y) for y in range(194) for x in range(w)
         if frame[y*pitch+x*4:y*pitch+x*4+4]!=native[y*pitch+x*4:y*pitch+x*4+4]]
 if pixels:
  visible+=1
  bounds=(min(x for x,y in pixels),min(y for x,y in pixels),max(x for x,y in pixels),max(y for x,y in pixels))
  assert bounds[2]-bounds[0]<32 and bounds[3]-bounds[1]<32,(label,bounds)
 checks+=1

mask_check('current QS1')
p.screenshot(ROOT/'diagnostics/appearance-current-qs1-verified.png')
for cycle in range(3):
 p.run(1,(3,));p.run(100);assert c.sh_coop_paused() and not c.sh_coop_appearance_available()
 p.run(1,(1,));p.run(2);assert not c.sh_coop_appearance_menu_get()&1
 p.screenshot(ROOT/f'diagnostics/appearance-current-qs1-map-{cycle}.png')
 p.run(1,(3,))
 for t in range(150):
  if t%5==0:mask_check(('map return',cycle,t))
  else:p.run(1)
 assert not c.sh_coop_paused() and c.sh_coop_appearance_get()==appearance
 r=p.ram()
 assert r[0x4d2e:0x4d4e]==r[0x4dae:0x4dce],'Native Rick palette did not settle after map exit'
 p.screenshot(ROOT/f'diagnostics/appearance-current-qs1-map-return-{cycle}.png')

# Reuse the controlled door fixture, loading native core and co-op halves.
raw=(ROOT/'diagnostics/doors.state').read_bytes();extra=(ROOT/'diagnostics/doors.extra').read_bytes()
c.retro_unserialize.argtypes=[C.c_void_p,C.c_size_t];c.retro_unserialize.restype=C.c_bool
buf=C.create_string_buffer(raw);assert c.retro_unserialize(buf,len(raw))
buf=C.create_string_buffer(extra);assert c.sh_coop_restore(buf,len(extra))
c.sh_coop_appearance_set(appearance)
exit_frames=entry_frames=exit_colored=entry_colored=0
for t in range(1000):
 p.buttons[1]={7} if t<260 else set()
 if t<260:
  # Probe.run's P1 argument is explicit; advance once then inspect the next frame.
  p.run(1,(7,))
 else:
  status=(C.c_uint*10)();c.sh_coop_status(status)
  p.run(1,(3,) if t%90==0 and not status[0] else ())
 r=p.ram();v=c.al_probe_vram();base=(c.sh_probe_vdp_regs()[5]&127)<<9
 link=0;follower=False
 for _ in range(80):
  a=base+link*8;attr=(v[(a+4)^1]<<8)|v[(a+5)^1]
  if c.sh_sprite_flags(link,attr)&3==2:follower=True
  link=v[(a+3)^1]&127
  if not link:break
 if follower and r[1]==4 and r[2] in (9,10):
  before=visible;mask_check(('door',r[2],t))
  if r[2]==10:exit_frames+=1;exit_colored+=visible-before
  else:entry_frames+=1;entry_colored+=visible-before
  p.screenshot(ROOT/('diagnostics/appearance-door-exit-verified.png' if r[2]==10 else 'diagnostics/appearance-door-entry-verified.png'))
 if t>300 and entry_frames and c.sh_coop_ready() and r[1:3]==bytes([4,10]) and not r[9]&64:break
assert exit_frames and entry_frames and exit_colored and entry_colored,(exit_frames,entry_frames,exit_colored,entry_colored)
assert c.sh_coop_appearance_get()==appearance
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
print('PASS current QS1 appearance',hex(appearance),'and 3 complete map cycles, native palette restoration, colored door exits/entries;',checks,'color checks; original save preserved')
