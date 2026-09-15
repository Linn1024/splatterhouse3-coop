"""Recover QuickSave6's stale room row without changing the source save."""
from probe import *
import zipfile
source=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'diagnostics/reported-y-save6.State'
original=source.read_bytes()
with zipfile.ZipFile(source) as z:w=z.read('Core.bin')
seed=w[4:4+int.from_bytes(w[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2);c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
def restore(state):p.restore(state);p.buttons[1]=set()
restore(seed);before=status();p.run(2);fixed=status()
assert fixed[5]==before[5]+512,(before,fixed)
assert fixed[2:5]==before[2:5] and fixed[6:]==before[6:],(before,fixed)
assert int.from_bytes(c.sh_probe_player(1)[0xa0:0xa4],'big')&0x4000,'Power form lost'
recovered=p.save();p.screenshot(ROOT/'diagnostics/y6-recovered.png')
def move():
 p.buttons[1]={5};p.run(32);down=status();assert down[5]>fixed[5]+8,(fixed,down)
 p.buttons[1]={4};p.run(16);up=status();assert up[5]<down[5],(down,up)
 assert up[2:4]==fixed[2:4] and up[6:]==fixed[6:],(fixed,up)
 return up,hashlib.sha256(p.frame[0]).hexdigest()
a=move();restore(recovered);assert move()==a
# Rebase a copy with fractional Y and a vertical pose offset. Compare against
# the otherwise identical valid context after one native update.
restore(recovered);q=c.sh_probe_player(1)
for address in (0x60a2,0x61a2):
 value=int.from_bytes(q[address:address+4],'big')+0x4321
 if address==0x60a2:value-=8<<16
 for i,b in enumerate(value.to_bytes(4,'big')):q[address+i]=b
reference=p.save();p.run(1);expected=(status(),bytes(c.sh_probe_player(1)[0x60a2:0x61a6]),hashlib.sha256(p.frame[0]).hexdigest())
restore(reference);q=c.sh_probe_player(1)
for address in (0x60a2,0x61a2):
 value=int.from_bytes(q[address:address+4],'big')-(512<<16)
 for i,b in enumerate(value.to_bytes(4,'big')):q[address+i]=b
p.run(1);actual=(status(),bytes(c.sh_probe_player(1)[0x60a2:0x61a6]),hashlib.sha256(p.frame[0]).hexdigest())
assert actual==expected,'Rebase changed fractional Y or vertical pose'
assert source.read_bytes()==original
print('PASS QuickSave6 recovers the correct room row, Up/Down work, blue/health/power stay intact, recovery and movement replay, fractional pose preserved, source unchanged')
