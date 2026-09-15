"""Exercise native floor-three room loading with a stale co-op context."""
from probe import *
import zipfile
source=ROOT/'diagnostics/reported-y-save6.State'
with zipfile.ZipFile(source) as z:w=z.read('Core.bin')
seed=w[4:4+int.from_bytes(w[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'));c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
p.restore(seed)
# Enter the native room-load phase directly, covering paths that do not run
# the earlier stage setup at 3002. No debug-stage helper resets co-op for us.
p.write(1,bytes([4,8]));p.write(9,b'\0');p.write(0x166,b'\2');p.write(0x16a,b'\xff');p.write(0x18e,b'\0')
for t in range(1600):
 p.run(1,(3,) if t%90==80 else ())
 if t>5 and p.ram()[1:3]==bytes([4,10]) and not(p.ram()[9]&64) and status()[0]:break
else:raise AssertionError('Third floor failed to start')
s=status();print('third floor',t,s)
assert s[4]==s[2]+32 and s[5]==s[3],('P2 retained previous floor context',s)
start=p.save()
p.buttons[1]={5};p.run(16);down=status();p.buttons[1]={4};p.run(16);up=status()
assert down[5]>s[5] and up[5]<down[5],(s,down,up)
print('PASS native third-floor load initializes red beside blue; vertical controls work without relying on Y-only recovery')
