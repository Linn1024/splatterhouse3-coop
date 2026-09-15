exec(compile(open(__file__.replace('trace_door.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
restore(((ROOT/'diagnostics/doors.state').read_bytes(),(ROOT/'diagnostics/doors.extra').read_bytes()))
import struct
last=None
for t in range(900):
 p.buttons[1]={7} if t<260 else set();p.run(1,(7,) if t<260 else ((3,) if t%90==0 else ()))
 r=p.ram();v=(r[1],r[2],r[9],r[0x18e],int.from_bytes(r[0xc2:0xc4],'big'),int.from_bytes(r[0x5520:0x5522],'big'))
 if v!=last or t%30==0:print(t,v,status()[0:6],hex(int.from_bytes(r[0x178:0x17c],'big')));last=v
 if 160<t<550 and t%20==0:p.screenshot(ROOT/f'diagnostics/door-trace-{t}.png')

