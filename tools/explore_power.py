exec(compile(open(__file__.replace('explore_power.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
for who in (0,1):
 for tier in (0,1,2,4):
  restore(base);p.buttons[1]=set()
  for i in (19,20):p.write(0x5f9e+i*4,(1500<<16).to_bytes(4,'big'))
  q=c.sh_probe_player(who);q[0xba]=0;q[0xbb]=40;q[0xbd]=tier
  if who==0:p.write(0xba,b'\0\x28');p.write(0xbd,bytes([tier]))
  seen=[]
  for t in range(150):
   p.buttons[1]={1} if who and t<5 else set();p.run(1,(1,) if not who and t<5 else ())
   if t in (0,30,59,60,90,149):
    q=c.sh_probe_player(who);seen.append((t,int.from_bytes(q[0xba:0xbc],'big'),q[0xbd],hex(int.from_bytes(q[0xa0:0xa4],'big'))))
  print(who,tier,seen)
