"""Door cinematics keep a separately colored, spaced P2 through the native script."""
exec(compile(open(__file__.replace('verify_door_animation.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
restore(((ROOT/'diagnostics/doors.state').read_bytes(),(ROOT/'diagnostics/doors.extra').read_bytes()))
seen_exit=seen_entry=False;entry=None;outgoing=None
for t in range(1000):
 p.buttons[1]={7} if t<260 else set();p.run(1,(7,) if t<260 else ((3,) if t%90==0 and not status()[0] else ()))
 r=p.ram();extra=save()[1];phase=r[2]
 # Ownership flag 2 marks the recolored native-frame door follower.
 count=sum((flag&3)==2 for flag in extra[144+0x550:144+0x5a0])
 if r[1]==4 and r[9]&64 and phase==10 and t<300 and count:
  sprites=[];index=0
  for _ in range(80):
   a=0x525e+index*8;item=r[a:a+8]
   sprites.append((extra[144+0x500+index],int.from_bytes(item[0:2],'big')&511,item[2],int.from_bytes(item[4:6],'big'),int.from_bytes(item[6:8],'big')&511))
   index=item[3]&127
   if not index:break
  red=[v for v in sprites if v[0]&3==2 and v[3]&0x6000==0x2000]
  blue=[v for v in sprites if v[0]&3==0 and v[3]&0x6000==0x2000]
  if red and blue:
   front,back=(blue,red) if extra[144+0x307]==1 else (red,blue)
   assert all(any(v[1:4]==b[1:4] and (v[4]-b[4])%512==32 for b in back) for v in front), ('Door entry spacing',red,blue)
   if outgoing is None:outgoing=save()
   seen_exit=True;p.screenshot(ROOT/'diagnostics/door-exit-duo.png')
 if r[1]==4 and phase==9 and count:
  seen_entry=True
  if entry is None:entry=save()
  p.screenshot(ROOT/'diagnostics/door-entry-duo.png')
 if t>300 and status()[0]:break
else:raise AssertionError('Door transition did not finish')
assert seen_exit and seen_entry,(seen_exit,seen_entry)
s=status();assert s[4]-s[2]==32,s
p.screenshot(ROOT/'diagnostics/door-arrival-duo.png')
restore(entry)
def replay():
 p.buttons[1]=set();p.run(20);return status(),hashlib.sha256(p.frame[0]).hexdigest()
a=replay();restore(entry);assert replay()==a
print('PASS: rightward entry has same Y / 32px X, unchanged arrival spacing, cinematic save replay')



# Exercise the other travel directions using the same native exit animation.
for direction in (1,4,8):
 restore(outgoing);c.sh_probe_player(0)[0x307]=0 # Legacy cinematic saves have no leader metadata.
 p.write(0x18e,bytes([direction]));p.buttons[1]=set();p.run(1)
 r=p.ram();extra=save()[1];sprites=[];index=0
 for _ in range(80):
  a=0x525e+index*8;item=r[a:a+8]
  sprites.append((extra[144+0x500+index],int.from_bytes(item[:2],'big')&511,item[2],int.from_bytes(item[4:6],'big'),int.from_bytes(item[6:8],'big')&511))
  index=item[3]&127
  if not index:break
 red=[v for v in sprites if v[0]&3==2 and v[3]&0x6000==0x2000]
 blue=[v for v in sprites if v[0]&3==0 and v[3]&0x6000==0x2000]
 dx=-32 if direction==1 else 0
 dy=40 if direction==4 else -40 if direction==8 else 0
 assert red and all(any(v[2:4]==b[2:4] and (v[4]-b[4])%512==dx%512 and (v[1]-b[1])%512==dy%512 for b in blue) for v in red),(direction,red,blue)
print('PASS: legacy cinematic saves: leftward entry separates X; up/down entry separates Y')
