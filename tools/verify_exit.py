exec(compile(open(__file__.replace('verify_exit.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
restore(((ROOT/'diagnostics/doors.state').read_bytes(),(ROOT/'diagnostics/doors.extra').read_bytes()))
p.buttons[1]={7}
for i in range(35):
 p.run(30,(7,));print(i,status(),p.ram()[0x166:0x180].hex(),p.ram()[9]);p.screenshot(ROOT/f'diagnostics/exit-{i}.png')


