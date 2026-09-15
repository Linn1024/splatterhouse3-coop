exec(compile(open(__file__.replace('verify_transform.py','verify_coop.py')).read().split('for who in (0,1):')[0],__file__,'exec'))
p2=c.sh_probe_player(1);p2[0xba]=0;p2[0xbb]=80
p.buttons[1]={1};p.run(240);print(status(), 'flags',bytes(c.sh_probe_player(1)[0xa0:0xa4]).hex(),p.ram()[0xf028:0xf02e].hex());p.screenshot(ROOT/'diagnostics/transform.png')
assert int.from_bytes(bytes(c.sh_probe_player(1)[0xa0:0xa4]),'big')&(1<<14)
assert not(int.from_bytes(bytes(c.sh_probe_player(0)[0xa0:0xa4]),'big')&(1<<14))
assert status()[8]==0 and status()[9]>0
