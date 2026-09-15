"""Regression for the unmodified BizHawk save that stalled at ROM PC 60C82."""
from probe import *
import zipfile
source=Path(sys.argv[1]) if len(sys.argv)>1 else Path(r'C:/TEMP2/Bizhawk/Libretro/State/splatterhouse_coop_libretro/Splatterhouse 3 (USA).QuickSave1.State')
original_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as archive: wrapped=archive.read('Core.bin')
size=int.from_bytes(wrapped[:4],'little');seed=wrapped[4:4+size]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
p.restore(seed);before=status();p.run(10);after=status();assert after[1]>before[1],(before,after)
p.screenshot(ROOT/'diagnostics/recovered-save.png')
for who in (0,1):
 p.restore(seed);p.run(10);start=status();p.buttons[1]={7} if who else set();p.run(20,{7} if not who else set());end=status()
 assert end[2+2*who]>start[2+2*who],(who,start,end)
 print('PASS recovered P'+str(who+1)+' movement',start[2+2*who],end[2+2*who])
p.buttons[1]=set();p.restore(seed);p.run(10);recovered=p.save()
def replay():
 p.buttons[1]={0};p.run(240,(0,));return status(),hashlib.sha256(p.frame[0]).hexdigest()
a=replay();p.restore(recovered);b=replay();assert a==b,(a,b);assert a[0][1]>after[1]+150,a
assert hashlib.sha256(source.read_bytes()).hexdigest()==original_hash
print('PASS original save resumes, both controls work, recovered save replays; original unchanged',a[0])
