exec(compile(open(__file__.replace('find_weapons.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
for level in range(6):
 c.sh_coop_debug_stage(level)
 for i in range(3200):
  p.run(1,(3,) if i%120==100 else ())
  if i>5 and c.sh_coop_ready() and c.sh_coop_level()==level:break
 p.run(30);r=p.ram();w=lambda a:int.from_bytes(r[a:a+2],'big')
 print('stage',level+1,[(i,w(0x581e+i*2),w(0x589e+i*2),w(0x5f9e+i*4),w(0x619e+i*4)) for i in range(2,64) if w(0x551e+i*2)&32768])
