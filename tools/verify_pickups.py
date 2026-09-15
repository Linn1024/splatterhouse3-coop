exec(compile(open(__file__.replace('verify_pickups.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
def w(a,v,n=2):p.write(a,int(v).to_bytes(n,'big'))
def spawn(who,kind):
 restore(base);p.buttons[1]=set()
 # Remove opponents to isolate native item behavior.
 for i in range(2,64):w(0x551e+i*2,0)
 p.run(2)
 q=c.sh_probe_player(who);x=int.from_bytes(q[0x5fa2:0x5fa4],'big');y=int.from_bytes(q[0x61a2:0x61a4],'big')
 a=0x551e+16;b=0x551e+32
 for o in list(range(0,0xa80,0x80))+list(range(0xe80,0x1100,0x80))+[0x1400,0x1480,0x1700,0x1780,0x1800,0x1880,0x1900]:w(a+o,0)
 for o in (0xa80,0xb80,0xc80,0xd80,0x1100,0x1200,0x1300,0x1500,0x1600):w(b+o,0,4)
 for o,v in ((0,0xc000),(0x80,1),(0x300,kind),(0x380,2),(0x580,0x100),(0x1080,4),(0xf80,14),(0x180,x-int.from_bytes(p.ram()[0xf0:0xf2],'big')+128),(0x200,y-int.from_bytes(p.ram()[0xf8:0xfa],'big')+128)):w(a+o,v)
 for o,v in ((0xa80,x+8),(0xb80,y),(0xc80,y)):w(b+o,v<<16,4)
 # Native stage-1 persistent weapon record index 6.
 w(0x731e+12,0x10ff)
 r=p.ram();end=r.index(255,0x54de,0x551e);p.write(end,b'\x10\xff')
 p.run(2)
for who in (0,1):
 for kind in (5,6,7):
  spawn(who,kind)
  for t in range(60):
   p.buttons[1]={0} if who and t%20<10 else set();p.run(1,{0} if not who and t%20<10 else set())
   q=c.sh_probe_player(who)
   if q[0xa3]&32:break
  print('pickup',who,kind,'frame',t,'flags',bytes(q[0xa0:0xa4]).hex(),'slot',q[0xd1],'obj',p.ram()[0x552e:0x5530].hex())
  assert q[0xa3]&32,'Native pickup failed'
  p.buttons[1]=set();p.run(20);p.screenshot(ROOT/f'diagnostics/held-p{who+1}-type{kind}.png')
  r=p.ram();a=0x552e
  print('held fields',[(hex(o),r[a+o:a+o+2].hex()) for o in (0,0x80,0x180,0x200,0x280,0x300,0x380)])
  assert r[a+0x80]&4, 'Held attachment flag missing'
  assert int.from_bytes(r[a+0x180:a+0x182],'big')==int.from_bytes(q[0x56a0:0x56a2],'big'), 'Weapon follows wrong Rick'
  seed=save();p.run(3);visible=p.frame[0];restore(seed);w(a,0);p.run(3);hidden=p.frame[0]
  changed=sum(x!=y for x,y in zip(visible,hidden));print('visible weapon bytes',changed);assert changed>15,'Held weapon not drawn'

