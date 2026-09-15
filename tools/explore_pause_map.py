from probe import *
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'));c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
for i in range(2400):
 p.run(1,(3,) if i%120==119 else ())
 if c.sh_coop_ready():break
p.run(3);p.run(1,(3,));p.run(1)
last=None
for t in range(250):
 p.run(1);r=p.ram();s=tuple(r[:5])
 if s!=last:print(t,s,c.sh_coop_paused());last=s
 if t in (30,90,150):p.screenshot(ROOT/f'diagnostics/map-{t}.png')
p.run(1,(3,));p.run(150);print('after',p.ram()[:5].hex(),c.sh_coop_paused())
