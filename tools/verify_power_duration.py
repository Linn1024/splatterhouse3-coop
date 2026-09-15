"""Power lasts through repeated depletion ticks, including legacy meter/tier mismatches."""
exec(compile(open(__file__.replace('verify_power_duration.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
for who in (0,1):
 for power,tier in ((20,1),(40,2),(80,4),(40,0),(40,1),(80,0)):
  restore(base);p.buttons[1]=set()
  for i in (19,20):p.write(0x5f9e+i*4,(1500<<16).to_bytes(4,'big'))
  q=c.sh_probe_player(who);q[0xba]=0;q[0xbb]=power;q[0xbd]=tier
  if who==0:p.write(0xba,bytes([0,power]));p.write(0xbd,bytes([tier]))
  for t in range(240):
   p.buttons[1]={1} if who and t<5 else set();p.run(1,(1,) if not who and t<5 else ())
   q=c.sh_probe_player(who)
   assert int.from_bytes(q[0xa0:0xa4],'big')&0x4000,(who,power,tier,t)
  remaining=int.from_bytes(q[0xba:0xbc],'big');assert power-5<=remaining<power,(power,remaining)
  partner=c.sh_probe_player(who^1);assert not(int.from_bytes(partner[0xa0:0xa4],'big')&0x4000)
  assert int.from_bytes(partner[0xba:0xbc],'big')==0
  # Exhaust the meter to verify that ordinary return-to-normal still works.
  q[0xba]=q[0xbb]=0
  if who==0:p.write(0xba,b'\0\0')
  p.buttons[1]=set();p.run(120)
  assert not(int.from_bytes(c.sh_probe_player(who)[0xa0:0xa4],'big')&0x4000)
 print('PASS P'+str(who+1)+': normal depletion, legacy tier repair, partner independence and exhausted-power return')
for who in (0,1):
 restore(base);p.buttons[1]=set();c.sh_coop_debug(6);p.run(2)
 for t in range(240):
  if t==20:
   for i in (19,20):p.write(0x551e+i*2,b'\0\0')
  p.buttons[1]={1} if who and t<5 else set();p.run(1,(1,) if not who and t<5 else ())
 q=c.sh_probe_player(who);assert p.ram()[0x168]&64
 assert int.from_bytes(q[0xa0:0xa4],'big')&0x4000,who
 assert int.from_bytes(q[0xba:0xbc],'big')>=75
 print('PASS P'+str(who+1)+': stays powered when room clears')
