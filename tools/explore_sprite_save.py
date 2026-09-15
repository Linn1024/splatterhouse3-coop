from probe import *
import zipfile,struct
p=Probe();c=p.core;c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
source=Path(r'C:/TEMP2/Bizhawk/Libretro/State/splatterhouse_coop_libretro/Splatterhouse 3 (USA).QuickSave1.State')
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')[4:];(ROOT/'diagnostics/sprite-save-original.bin').write_bytes(d)
h=struct.unpack('<8I',d[:32]);p.restore(d[32:32+h[2]]);extra=C.create_string_buffer(d[32+h[2]:32+h[2]+h[3]]);c.sh_coop_restore(extra,h[3])
for t in range(60):
 p.run(1)
 if t in (0,1,5,15,30,59):
  p.screenshot(ROOT/f'diagnostics/sprite-save-{t}.png')
  print(t,[(bytes(c.sh_probe_player(w)[0xa0:0xa4]).hex(),[(hex(o),int.from_bytes(c.sh_probe_player(w)[0x5520+o:0x5522+o],'big')) for o in (0x280,0x600,0x680,0x780)]) for w in (0,1)])
