from probe import *
import zipfile,struct
p=Probe(Path(sys.argv[1]) if len(sys.argv)>1 else CORE);c=p.core;c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
with zipfile.ZipFile(r'C:/TEMP2/Bizhawk/Libretro/State/splatterhouse_coop_libretro/Splatterhouse 3 (USA).QuickSave1.State') as z:d=z.read('Core.bin')[4:]
h=struct.unpack('<8I',d[:32]);p.restore(d[32:32+h[2]]);extra=C.create_string_buffer(d[32+h[2]:32+h[2]+h[3]]);c.sh_coop_restore(extra,h[3])
def report(t):
 print(t,'roomflags',hex(p.ram()[0x168]),'cheats',c.sh_coop_debug_flags(),[(bytes(c.sh_probe_player(w)[0xa0:0xa4]).hex(),bytes(c.sh_probe_player(w)[0xba:0xbe]).hex()) for w in (0,1)])
report(0)
for t in range(240):
 p.buttons[1]={1} if t<5 else set();p.run(1)
 if t in (1,30,60,90,120,180,239):report(t)
