"""Powered recovery pose: the mask must not color chest/shoulder glints."""
from probe import *
from appearance_probe import actor_pixels,gray_components
from native_rick_pose import stage_pose
import zipfile

source=ROOT/'diagnostics/appearance-chest-qs1.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
seed=d[4:4+int.from_bytes(d[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
images=[]
native=[0,0x080808,0xeeeecc,0xbbbbad,0x808080,0x444444,0xffdd88,0xddaa55,
        0xaa7733,0x704400,0x448888,0x226666,0x114444,0x082222,0x990000,0x555500]
for frame in (0x99,0xe6):
 p.restore(seed);p.run(1);stage_pose(c,frame);c.sh_coop_appearance_set(0x040501)
 raw=actor_pixels(c);x0=min(x for x,y in raw);x1=max(x for x,y in raw);y0=min(y for x,y in raw)
 largest=gray_components(raw,True)[0]
 # The native drawing's chin and shoulder boundary, annotated independently
 # of the connected gray surface which also includes 55 skin pixels.
 limits={14:32,15:32,16:31,17:30}
 expected={xy for xy in largest if xy[1]-y0<14 or
           (xy[1]-y0<18 and (x1-xy[0] if frame==0xe6 else xy[0]-x0)<=limits[xy[1]-y0])}
 c.sh_coop_material_prepare()
 actual={xy for xy,i in raw.items() if 2<=i<=5 and not c.sh_coop_material_ink(*xy,i)&16}
 assert actual==expected,(hex(frame),actual^expected)
 assert len(expected)==152 and len(largest-expected)==55
 # Restore and re-stage the pose: classification must not depend on a previous
 # pose or the material cache surviving a save load.
 stage_pose(c,frame);c.sh_coop_material_prepare()
 assert {xy for xy,i in raw.items() if 2<=i<=5 and not c.sh_coop_material_ink(*xy,i)&16}==expected
 im=Image.new('RGB',(50,76))
 for xy,i in raw.items():
  tagged=c.sh_coop_material_ink(*xy,i);color=c.sh_coop_appearance_rgb(tagged,native[i])
  if xy in largest-expected and i<=3:assert color==native[i],'Pale skin highlights lost'
  im.putpixel((xy[0]-x0,xy[1]-y0),((color>>16)&255,(color>>8)&255,color&255))
 images.append(im.resize((300,456),Image.Resampling.NEAREST))
 print('PASS powered recovery',hex(frame),'152 mask pixels; 55 touching skin pixels excluded')
sheet=Image.new('RGB',(600,456))
for i,im in enumerate(images):sheet.paste(im,(i*300,0))
sheet.save(ROOT/'diagnostics/powered-recovery-fixed.png')
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
print('PASS both native facings; source save unchanged')
