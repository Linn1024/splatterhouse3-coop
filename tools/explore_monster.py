from probe import *
import zipfile,struct
p=Probe();c=p.core;c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
source=Path(r'C:/TEMP2/Bizhawk/Libretro/State/splatterhouse_coop_libretro/Splatterhouse 3 (USA).QuickSave1.State')
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')[4:];(ROOT/'diagnostics/invincible-monster-save.bin').write_bytes(d)
h=struct.unpack('<8I',d[:32]);p.restore(d[32:32+h[2]]);b=C.create_string_buffer(d[32+h[2]:32+h[2]+h[3]]);c.sh_coop_restore(b,h[3])
def dump(t):
 r=p.ram();print('xys',[(int.from_bytes(c.sh_probe_player(w)[0x5fa2:0x5fa4],'big'),int.from_bytes(c.sh_probe_player(w)[0x61a2:0x61a4],'big')) for w in (0,1)]);print('t',t,'cheats',c.sh_coop_debug_flags(),'players',[(bytes(c.sh_probe_player(w)[0xa0:0xa4]).hex(),int.from_bytes(c.sh_probe_player(w)[0x190:0x192],'big')) for w in (0,1)])
 for i in range(19,40):
  a=0x551e+i*2
  if r[a]&128:print(i,[(hex(o),r[a+o:a+o+2].hex()) for o in (0,0x80,0x280,0x300,0x380,0x980,0x1080)],int.from_bytes(r[0x5f9e+i*4:0x5fa0+i*4],'big'),int.from_bytes(r[0x619e+i*4:0x61a0+i*4],'big'))
dump(0)
p.buttons[1]={6};p.run(14,(6,));dump(14)
for t in range(400):
 p.buttons[1]={0} if t%20<10 else set();p.run(1,(0,) if t%20<10 else ())
 if t in (1,100,399):dump(t);p.screenshot(ROOT/f'diagnostics/monster-{t}.png')

r=p.ram();a=0x551e+38;b=0x551e+76
print('fields',[(hex(o),r[a+o:a+o+2].hex()) for o in range(0,0x1980,0x80)])
print('longs',[(hex(o),r[b+o:b+o+4].hex()) for o in (0x1100,0x1200,0x1300,0x1500,0x1600)])
