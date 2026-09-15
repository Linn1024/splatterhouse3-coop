"""Walk the user's two real doors with either Rick arriving first; never edit the save."""
from probe import *
import zipfile
source=Path(sys.argv[1]) if len(sys.argv)>1 else Path(r'C:/TEMP2/Bizhawk/Libretro/State/splatterhouse_coop_libretro/Splatterhouse 3 (USA).QuickSave1.State')
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as z:w=z.read('Core.bin')
seed=w[4:4+int.from_bytes(w[:4],'little')]
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll');c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)
c.sh_coop_save.argtypes=[C.c_void_p,C.c_uint]
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
def extra():
 b=C.create_string_buffer(c.sh_coop_state_size());assert c.sh_coop_save(b,len(b));return b.raw[144:]
def step(buttons):
 p.buttons[1]=buttons[1];p.run(1,buttons[0])
def steer(who,target,axis):
 s=status();x,y=s[2+who*2:4+who*2];tx,ty=target
 if axis=='x' and abs(x-tx)>8:return {7 if x<tx else 6}
 if abs(y-ty)>2:return {5 if y<ty else 4}
 if abs(x-tx)>8:return {7 if x<tx else 6}
 return set()
def sprites():
 r=p.ram();e=extra();pieces=[[],[]];index=0
 for _ in range(80):
  a=0x525e+index*8;item=r[a:a+8];flags=e[0x500+index]
  if flags&8 and int.from_bytes(item[4:6],'big')&0x6000==0x2000:
   pieces[bool(flags&3)].append((int.from_bytes(item[6:8],'big')&511,int.from_bytes(item[:2],'big')&511,item[2],int.from_bytes(item[4:6],'big')))
  index=item[3]&127
  if not index:break
 return pieces
for name,target,axis,press,direction in [('right',(1264,1184),'y',7,2),('up',(1152,1168),'x',4,8)]:
 for leader in (0,1):
  p.restore(seed);step([set(),set()]);initial=status();room=p.ram()[0x178:0x17c]
  assert initial[2:6]==[1152,1192,1128,1212], 'QuickSave1 changed: this regression needs the September 15 two-door room save'
  triggered=False;paired=single=False;checkpoint=None;exit_frames=0;fade_frames=0;trace=[]
  for t in range(1200):
   buttons=[set(),set()]
   if not triggered:
    buttons[leader]=steer(leader,target,axis) or {press}
    if t>=400:buttons[1-leader]=steer(1-leader,target,axis) or {press}
   step(buttons);r=p.ram();s=status()
   if triggered and r[1]==5 and r[3]==4:
    fade_frames+=1;e=extra()
    for index,flags in enumerate(e[0x550:0x5a0]):
     if flags&8:
      assert c.sh_sprite_flags(index,0x2000)==flags,(name,leader,'Ownership lost during door fade',index,flags)
   if r[9]&64 and r[0x18e]==direction and int.from_bytes(r[0xc2:0xc4],'big')<=4:
    assert t>=400, 'Door opened without the partner'
    triggered=True;exit_frames+=1;assert extra()[0x307]==leader+1,(name,leader,'wrong leader')
    pieces=sprites();front,back=pieces[leader],pieces[1-leader]
    if single and (front or back):
     assert back and not front,(name,leader,'Follower changed color on final entry frame',int.from_bytes(r[0xc2:0xc4],'big'))
    if front and back:
     dx,dy=(32,0) if direction==2 else (0,-40)
     assert all(any(a[2:]==b[2:] and (a[0]-b[0])%512==dx%512 and (a[1]-b[1])%512==dy%512 for b in back) for a in front),(name,leader,front,back)
     if exit_frames>=3:p.screenshot(ROOT/f'diagnostics/checked-door-{name}-{leader}-pair.png')
     paired=True
    if back and not front:
     if single:p.screenshot(ROOT/f'diagnostics/checked-door-{name}-{leader}-follower.png')
     single=True
    if exit_frames==3:checkpoint=p.save()
   if triggered and r[1]==4 and r[2]==9:
    (ROOT/f'diagnostics/checked-door-{name}-{leader}-emerging.bin').write_bytes(p.save())
   if checkpoint and exit_frames>=3:
    trace.append((s,hashlib.sha256(p.frame[0]).hexdigest()))
   if triggered and s[0] and not r[9]&64:
    assert r[0x178:0x17c]!=room
    assert fade_frames>0,'Door test missed the fade'
    assert s[6:]==initial[6:],(name,leader,'health/power changed',initial,s)
    p.screenshot(ROOT/f'diagnostics/checked-door-{name}-{leader}-arrival.png')
    (ROOT/f'diagnostics/checked-door-{name}-{leader}-arrival.bin').write_bytes(p.save())
    break
  else:raise AssertionError((name,leader,'transition stalled',status()))
  assert paired and single and exit_frames>20,(name,leader,paired,single,exit_frames)
  p.restore(checkpoint)
  for expected in trace[1:]:
   step([set(),set()]);assert (status(),hashlib.sha256(p.frame[0]).hexdigest())==expected,(name,leader,'save replay')
  before=status();step([set(),set()]);p.buttons[1]={6};p.run(15,(6,));after=status()
  assert after[2]<before[2] and after[4]<before[4],(name,leader,'controls after arrival',before,after)
  print('PASS',name,('blue','red')[leader]+' first:',exit_frames,'entry frames, correct sprite order/spacing, follower finishes, room/health/power/controls and save replay')
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
print('PASS original QuickSave1 unchanged',source_hash)



