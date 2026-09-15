"""Genesis X kills the native room enemy group, once per press."""
from probe import *
import zipfile
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'));c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
def load(slot):
 with zipfile.ZipFile(ROOT/f'diagnostics/reported-sprites-save{slot}.State') as z:w=z.read('Core.bin')
 p.restore(w[4:4+int.from_bytes(w[:4],'little')]);p.buttons[1]=set()
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
for slot in (2,3):
 for who in (0,1):
  load(slot);p.run(2);initial=status()
  p.buttons[1]={10} if who else set();p.run(1,() if who else (10,));r=p.ram()
  assert r[0x5ec4:0x5ec6]==b'\0\0','Enemy did not receive fatal hit'
  mid=p.save()
  def finish():
   p.buttons[1]={10} if who else set();p.run(1000,() if who else (10,));r=p.ram()
   assert not(int.from_bytes(r[0x5544:0x5546],'big')&0x8000),(slot,who,'Enemy remained alive')
   if slot==2:
    assert not(int.from_bytes(r[0x5546:0x5548],'big')&0x8000)
    assert status()[6:8]==initial[6:8],'Cheat harmed a player'
   else:assert r[1]!=4 or r[2]!=10,'Boss death did not advance the stage'
   return status(),hashlib.sha256(p.frame[0]).hexdigest()
  a=finish();p.restore(mid);assert finish()==a
  print('PASS save',slot,'P'+str(who+1),'X: all enemies die, holding does not restart death, progression and held-button save replay')
load(1);p.run(2);r=p.ram();hp=r[0x5ec4:0x5ec8];p.run(30,(10,));assert p.ram()[0x5ec4:0x5ec8]==hp,'Cheat ran on the map'
load(5);p.run(2);r=p.ram();objects=[r[0x551e+i*2:0x5520+i*2] for i in (5,6,7)];p.run(1,(10,));assert objects==[p.ram()[0x551e+i*2:0x5520+i*2] for i in (5,6,7)],'Cheat removed doors'
print('PASS ignored on map; doors retained in empty room')
