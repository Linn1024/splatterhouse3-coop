"""Keep frontend geometry fixed through native video-mode and save changes."""
from probe import *
import struct
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.sh_probe_vdp_regs.restype=C.POINTER(C.c_ubyte)
seen=set()
def tick(buttons=()):
 p.run(1,buttons)
 assert p.frame[1:]==(320,254,1280),p.frame[1:]
 seen.add(320 if c.sh_probe_vdp_regs()[12]&1 else 256)
for i in range(2400):
 tick((3,) if i%120==119 else ())
 if c.sh_coop_ready():break
else:raise AssertionError('game did not start')
for i in range(30):tick()
seed=p.save();native=struct.unpack('<8I',seed[:32]);assert native[5:7]==(256,224),native
p.screenshot(ROOT/'diagnostics/fixed-gameplay-size.png')
tick((3,))
for _ in range(90):tick()
for buttons in ((0,1,8),()):tick(buttons)
p.screenshot(ROOT/'diagnostics/fixed-debug-size.png')
p.restore(seed);tick()
# Full stage loading transitions, retaining the fixed frontend surface.
for stage in (1,4,5):
 assert c.sh_coop_debug_stage(stage)
 for i in range(3200):
  tick((3,) if i%120==119 and not c.sh_coop_ready() else ())
  if i>5 and c.sh_coop_ready() and c.sh_coop_level()==stage:break
 else:raise AssertionError(('stage failed',stage))
p.restore(seed);tick();p.core.retro_reset()
for i in range(180):tick()
assert seen=={256,320},seen
class Geometry(C.Structure):_fields_=[('base_width',C.c_uint),('base_height',C.c_uint),('max_width',C.c_uint),('max_height',C.c_uint),('aspect',C.c_float)]
class Timing(C.Structure):_fields_=[('fps',C.c_double),('rate',C.c_double)]
class AV(C.Structure):_fields_=[('geometry',Geometry),('timing',Timing)]
av=AV();p.core.retro_get_system_av_info(C.byref(av));g=av.geometry
assert (g.base_width,g.base_height,g.max_width,g.max_height)==(320,254,320,254)
assert abs(g.aspect - (4/3)*224/254)<0.00001
print('PASS: fixed 320x254 frames and geometry across native 256/320 modes, stage transitions, debug, save/load and reset')
