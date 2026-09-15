"""Reported boss palette recovery and native/co-op grapple comparison."""
from probe import *
import zipfile,struct
sources=[ROOT/f'diagnostics/boss-review-{i}.State' for i in (1,2)]
original=[s.read_bytes() for s in sources];seeds=[]
for source in sources:
 with zipfile.ZipFile(source) as z:w=z.read('Core.bin')
 seeds.append(w[4:4+int.from_bytes(w[:4],'little')])
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'));c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
for slot in (1,2):
 p.restore(seeds[slot-1]);p.buttons[1]=set();expected=bytes(p.ram()[0x4dee:0x4e0e]);p.run(3)
 assert p.ram()[0x4d6e:0x4d8e]==expected,'Saved boss palette not recovered'
 p.screenshot(ROOT/f'diagnostics/boss-palette-fixed-{slot}.png');p.run(1,(3,));p.run(90);assert p.ram()[1:3]==bytes([11,3]);paused=p.save()
 def resume():
  p.run(1,(3,));p.run(150);assert p.ram()[0x4d6e:0x4d8e]==expected,'Map damaged boss palette'
  return hashlib.sha256(p.frame[0]).hexdigest(),p.ram()
 a=resume();p.restore(paused);assert resume()==a
 # Earlier map snapshots have no stored palette block.
 legacy=bytearray(paused);h=struct.unpack('<8I',legacy[:32]);e=32+h[2]+144
 legacy[e+0x308]=1;legacy[e+0x70a0:e+0x71a0]=bytes(256)
 p.restore(bytes(legacy));resume()
 print('PASS save',slot,'boss palette recovered; map/save replay and older paused saves keep correct colors')
# Compare positions and grapple state against the untouched original-game logic.
traces=[]
for enabled in (False,True):
 p.restore(seeds[1]);p.buttons[1]=set()
 if not enabled:c.sh_coop_enable(0)
 trace=[]
 for t in range(720):
  p.run(1,(0,) if t>=120 and (t-120)%20<10 else ())
  r=p.ram();trace.append((bytes(r[0x5fea:0x5fee]),bytes(r[0x61ea:0x61ee]),bytes(r[0x190:0x192]),bytes(r[0x5ec4:0x5ec6])))
 traces.append(trace)
release=[next(i for i,x in enumerate(trace) if x[2]==b'\0\0') for trace in traces]
assert release[0]==release[1]
assert traces[0][:release[0]+1]==traces[1][:release[1]+1], 'Held/release behavior differs from original'
assert len(set(x[:2] for x in traces[1][:120]))==1
assert all(x[2]==b'\0\x26' for x in traces[1][:120])
assert any(x[2]==b'\0\0' for x in traces[1][120:])
assert len(set(x[:2] for x in traces[1][120:]))>1
assert [s.read_bytes() for s in sources]==original
print('PASS QS2 stationary while held, attacks release it and it moves; held/release trace matches original game; source saves unchanged')

