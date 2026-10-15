"""Ultra Hard: actual enemy hits, both power meters, death, and save persistence."""
exec(compile(open(__file__.replace('verify_ultra_hard.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
def toggle_ultra():
 q=c.sh_probe_player(0);q[0x301]^=16;return True
hits=[]
for ultra in (False,True):
 restore(base);p.buttons[1]=set()
 if ultra:assert toggle_ultra()
 last=status()[6:8];seen={}
 for t in range(1500):
  p.run(1);hp=status()[6:8]
  for who in (0,1):
   if hp[who]<last[who] and who not in seen:seen[who]=(t,last[who]-hp[who])
  last=hp
  if len(seen)==2:break
 assert len(seen)==2
 hits.append(seen)
for who in (0,1):
 assert hits[1][who]==(hits[0][who][0],2*hits[0][who][1])
print('PASS both players take twice the native damage on the same enemy hit')
for who in (0,1):
 remaining=[]
 for ultra in (False,True):
  restore(base);p.buttons[1]=set()
  for slot in (19,20):p.write(0x5f9e+slot*4,(1500<<16).to_bytes(4,'big'))
  assert c.sh_coop_debug(6)
  if ultra:assert toggle_ultra()
  p.run(2)
  p.buttons[1]={1} if who else set();p.run(1,() if who else (1,))
  p.buttons[1]=set();p.run(239)
  start=status()[8+who];p.run(240)
  remaining.append(start-status()[8+who])
  assert int.from_bytes(bytes(c.sh_probe_player(who)[0xa0:0xa4]),'big')&0x4000
 assert remaining==[4,8],remaining
 print('PASS P'+str(who+1),'power consumed over 240 updates, original/Ultra:',remaining)
restore(base);p.buttons[1]=set();assert toggle_ultra()
# Lethal damage follows the native death path, without unsigned HP wrap.
q=c.sh_probe_player(1);q[0xb2]=0;q[0xb3]=4
for t in range(100):
 p.run(1)
 if status()[7]==0:break
else:raise AssertionError('Expected a lethal hit')
assert int.from_bytes(bytes(q[0xa0:0xa4]),'big')&0x8000
seed=save()
assert toggle_ultra() and not c.sh_coop_debug_flags()&16
restore(seed);assert c.sh_coop_debug_flags()&16
print('PASS native lethal-hit handling and Ultra setting save/load')
