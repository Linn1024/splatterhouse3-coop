"""Reported QS1 and cold-start mask isolation across moving/attacking poses."""
from probe import *
import zipfile

source=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'diagnostics/appearance-reported-qs1.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:data=z.read('Core.bin')
reported=data[4:4+int.from_bytes(data[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
checks=visible=0;poses=set()

def check_mask(label,buttons=()):
 global checks,visible
 seed=p.save()
 p.buttons[1]=set(buttons)
 # Replay the native control twice so lazy native tile-cache reconstruction
 # after loading cannot be mistaken for a material-color difference.
 for _ in range(2):
  p.restore(seed);c.sh_coop_appearance_set(0x030001);p.run(1)
 native=p.frame[0]
 p.restore(seed);c.sh_coop_appearance_set(0x030201);p.run(1)
 frame,w,h,pitch=p.frame
 changed=[(x,y) for y in range(194) for x in range(w)
          if frame[y*pitch+x*4:y*pitch+x*4+4]!=native[y*pitch+x*4:y*pitch+x*4+4]]
 if changed:
  visible+=1
  bounds=(min(x for x,y in changed),min(y for x,y in changed),max(x for x,y in changed),max(y for x,y in changed))
  if bounds[2]-bounds[0]>=32 or bounds[3]-bounds[1]>=32:
   p.screenshot(ROOT/'diagnostics/appearance-mask-leak.png')
   Image.frombytes('RGB',(w,h),native,'raw','BGRX',pitch).save(ROOT/'diagnostics/appearance-mask-leak-native.png')
   (ROOT/'diagnostics/appearance-mask-leak.seed').write_bytes(seed)
  assert bounds[2]-bounds[0]<32 and bounds[3]-bounds[1]<32,(label,'Mask color leaked onto the body',bounds)
 checks+=1
 poses.add(int.from_bytes(c.sh_probe_player(1)[0x57a0:0x57a2],'big'))

p.restore(reported);check_mask('QS1 first displayed frame')
p.screenshot(ROOT/'diagnostics/appearance-qs1-fixed.png')
for t in range(240):
 buttons=({0} if t%18<9 else set())|({6} if (t//40)%2 else {7})
 check_mask(('QS1 animation',t),buttons)
assert len(poses)>=8,poses
print('PASS QS1 mask stays on the face across',len(poses),'normal attack/movement poses')

# A full reset/cold campaign start must also classify the entrance sprites.
p.core.retro_reset();p.buttons[1]=set();c.sh_coop_appearance_set(0x030201)
for t in range(2400):
 p.run(1,(3,) if t%120==119 else ())
 if c.sh_coop_ready():break
else:raise AssertionError('Campaign did not start')
for t in range(180):check_mask(('Cold start',t),() if t<90 else ({0} if t%18<9 else {6}))
p.screenshot(ROOT/'diagnostics/appearance-cold-start-fixed.png')
assert checks==421 and visible>300,(checks,visible)
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash,'Source quicksave changed'
print('PASS cold entrance and later animation frames; all',checks,'mask comparisons; source QS1 preserved')
