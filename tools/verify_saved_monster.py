"""The purple monster from QuickSave1 must leave stale collision exclusion and die normally."""
from probe import *
import zipfile,struct
source=Path(r'C:/TEMP2/Bizhawk/Libretro/State/splatterhouse_coop_libretro/Splatterhouse 3 (USA).QuickSave1.State');digest=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as archive:d=archive.read('Core.bin')[4:]
h=struct.unpack('<8I',d[:32]);native=d[32:32+h[2]];extra=d[32+h[2]:32+h[2]+h[3]]
p=Probe();c=p.core
for who in (0,1):
 p.restore(native);b=C.create_string_buffer(extra);assert c.sh_coop_restore(b,len(extra))
 r=p.ram();assert r[0x5ec4:0x5ec6]==b'\0\x10' # slot 19 health 16
 p.buttons[1]={6};p.run(40,(6,));p.buttons[1]=set()
 for t in range(500):
  p.buttons[1]={0} if who and t%20<10 else set();p.run(1,(0,) if not who and t%20<10 else ())
  if not p.ram()[0x5544]&128:break
 else:raise AssertionError(('Monster remained invincible',who))
 assert p.ram()[0x5ec4:0x5ec6]==b'\0\0'
 print('PASS: P'+str(who+1)+' defeats saved monster through normal attacks in',t,'frames')
assert hashlib.sha256(source.read_bytes()).hexdigest()==digest

