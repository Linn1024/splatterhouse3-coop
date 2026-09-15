"""Mixed Rick forms must not change the idle partner's visible sprite."""
exec(compile(open(__file__.replace('verify_mixed_forms.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
# Keep enemies alive (the native game blocks power in a cleared room), far away.
for i in (19,20):p.write(0x5f9e+i*4,(1500<<16).to_bytes(4,'big'))
p.buttons[1]={7};p.run(55);p.buttons[1]=set();p.run(20)
c.sh_coop_debug(6);p.run(2);mixed_base=save()
c.al_probe_vram.restype=C.POINTER(C.c_ubyte)
def partner_tiles(who):
 if who==0:return bytes(c.al_probe_vram()[:0xc00])
 return save()[1][-0xc00:]
def transformed(who):return bool(int.from_bytes(bytes(c.sh_probe_player(who)[0xa0:0xa4]),'big')&0x4000)
# Reference the normal idle animation's two banks, including partial uploads.
reference=[[set(),set()],[set(),set()]]
for t in range(500):
 p.buttons[1]=set();p.run(1)
 for partner in (0,1):
  art=partner_tiles(partner)
  for bank in (0,1):reference[partner][bank].add(art[bank*0x600:(bank+1)*0x600])
def intact(partner):
 art=partner_tiles(partner)
 return all(art[bank*0x600:(bank+1)*0x600] in reference[partner][bank] for bank in (0,1))

for who in (0,1):
 restore(mixed_base);p.buttons[1]=set();p.run(3);partner=who^1;p.screenshot(ROOT/'diagnostics/mixed-before.png')
 p.buttons[1]={1} if who else set();p.run(60,() if who else (1,));p.buttons[1]=set()
 print('form',who,status(),bytes(c.sh_probe_player(who)[0xa0:0xa4]).hex());assert transformed(who) and not transformed(partner)
 p.screenshot(ROOT/'diagnostics/mixed-after.png');assert intact(partner),('Partner sprite changed',who)
 p.screenshot(ROOT/f'diagnostics/mixed-form-p{who+1}.png')
 for i in range(60):
  p.buttons[1]={0} if who and i%20<10 else set();p.run(1,{0} if not who and i%20<10 else ())
  assert intact(partner),('Partner sprite corrupted during powered attack',who,i)
 seed=save()
 def replay():
  p.buttons[1]=set();p.run(40,(7,));return status(),hashlib.sha256(p.frame[0]).hexdigest()
 a=replay();restore(seed);assert replay()==a
 restore(seed);p.buttons[1]=set()
 # Exhaust this Rick's power and let the native return animation run.
 q=c.sh_probe_player(who);q[0xba]=q[0xbb]=q[0xbd]=0
 if who==0:p.write(0xba,b'\0\0');p.write(0xbd,b'\0')
 p.run(120)
 assert not transformed(who),'Failed to leave power mode'
 assert intact(partner),('Partner corrupted leaving power',who)
 print('PASS: P'+str(who+1)+' transforms, attacks, returns to normal; partner animation banks remain valid; save replay passes')
restore(mixed_base);p.buttons[1]={1};p.run(60,(1,));assert transformed(0) and transformed(1);p.buttons[1]=set();p.run(20);p.screenshot(ROOT/'diagnostics/both-powered.png')
print('PASS: both can transform together')

