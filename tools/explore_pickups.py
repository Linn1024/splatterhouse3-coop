from verify_room_gate import *
r=p.ram();w=lambda a:int.from_bytes(r[a:a+2],'big')
for i in range(1,64):
 a=0x551e+2*i
 if w(a)&0x8000:print(i,hex(w(a)),w(a+0x300),w(a+0x380),w(0x5f9e+i*4),w(0x619e+i*4),hex(w(a+0x280)))
st=save();(ROOT/'diagnostics/pickup-room.state').write_bytes(st[0]);(ROOT/'diagnostics/pickup-room.extra').write_bytes(st[1]);p.screenshot(ROOT/'diagnostics/pickup-room.png')
