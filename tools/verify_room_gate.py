exec(compile(open(__file__.replace('verify_room_gate.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
restore(((ROOT/'diagnostics/doors.state').read_bytes(),(ROOT/'diagnostics/doors.extra').read_bytes()))
p2=c.sh_probe_player(1);p2[0xb2]=0;p2[0xb3]=180;p2[0xba]=0;p2[0xbb]=40;p2[0xbd]=2;p2[0xbc]=17
p.buttons[1]={7};p.run(200,(7,));p.run(150)
assert status()[0],status()
assert p.ram()[0x178:0x17c]==bytes.fromhex('0000e1ee')
assert not(p.ram()[9]&64)
assert status()[2]<700 and status()[4]>=728,status()
print('PASS: P2 at exit waits for distant P1',status())
p.buttons[1]={7};p.run(70,(7,));middle=save()
def finish():
 for i in range(40):
  p.buttons[1]=set();p.run(90);p.run(1,(3,))
  if status()[0] and status()[2]>800:break
 else:raise AssertionError(('next room never loaded',status()))
 return status(),hashlib.sha256(p.frame[0]).hexdigest()
a=finish();assert a[0][7]==180 and a[0][9]==40,a
restore(middle);b=finish();assert a==b,(a,b)
assert c.sh_probe_player(1)[0xbd]==2 and c.sh_probe_player(1)[0xbc]==17, 'P2 tier/timer lost crossing door'
p.screenshot(ROOT/'diagnostics/room-gate.png')
print('PASS: both join exit, next room preserves P2 health/power, transition save replays',a[0])
