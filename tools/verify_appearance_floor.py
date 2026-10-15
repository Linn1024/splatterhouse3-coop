"""Powered floor poses keep their rotated mask separate from skin."""
from probe import *
from appearance_probe import actor_pixels,gray_components
from native_rick_pose import stage_pose
import zipfile

source=ROOT/'diagnostics/appearance-floor-qs1.State'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:d=z.read('Core.bin')
seed=d[4:4+int.from_bytes(d[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
lefts=([59,63,63,61,60,60,59,52,53,55,55,55,54,54,53,54,55,53],
       [61,62,62,62,61,61,60,59,58,56,58,60,61,62,62,63,63,63,63,63])
native=[0x000000,0x080808,0xeeeecc,0xbbbbad,0x808080,0x444444,
        0xffdd88,0xddaa55,0xaa7733,0x704400,0x448888,0x226666,0x114444,0x082222,0x990000,0x555500]
images=[]
for frame in (0x98,0x9d,0xe5,0xea):
 p.restore(seed);p.run(1);stage_pose(c,frame);c.sh_coop_appearance_set(0x040501)
 raw=actor_pixels(c);x0=min(x for x,y in raw);x1=max(x for x,y in raw);y0=min(y for x,y in raw)
 pose=frame in (0x9d,0xea);mirrored=frame>=0xe5;top=6 if pose else 24;left=lefts[pose]
 expected={xy for xy,i in raw.items() if 2<=i<=5 and top<=xy[1]-y0<top+len(left)
           and (x1-xy[0] if mirrored else xy[0]-x0)>=left[xy[1]-y0-top]}
 c.sh_coop_material_prepare()
 actual={xy for xy,i in raw.items() if 2<=i<=5 and not c.sh_coop_material_ink(*xy,i)&16}
 assert actual==expected,(hex(frame),'Mask/skin boundary',actual^expected)
 assert len(expected)==(120 if pose else 135),(hex(frame),len(expected))
 excluded=gray_components(raw,True)[0]-expected
 assert len(excluded)==(41 if pose else 27)
 im=Image.new('RGB',(90,60))
 for xy,ink in raw.items():
  tagged=c.sh_coop_material_ink(*xy,ink);color=c.sh_coop_appearance_rgb(tagged,native[ink])
  if xy in excluded and ink<=3:assert color==native[ink],'Skin lost its pale highlights'
  im.putpixel((xy[0]-x0,xy[1]-y0),((color>>16)&255,(color>>8)&255,color&255))
 images.append(im.resize((360,240),Image.Resampling.NEAREST))
 print('PASS powered floor pose',hex(frame),len(expected),'mask pixels;',len(excluded),'touching skin pixels excluded')
sheet=Image.new('RGB',(720,480))
for i,im in enumerate(images):sheet.paste(im,((i%2)*360,(i//2)*240))
sheet.save(ROOT/'diagnostics/powered-floor-colors-fixed.png')
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
print('PASS all four native floor poses; source save unchanged')
