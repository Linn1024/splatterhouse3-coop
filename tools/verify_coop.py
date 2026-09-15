from probe import *
p=Probe();c=p.core
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
c.sh_coop_save.argtypes=[C.c_void_p,C.c_uint];c.sh_coop_restore.argtypes=[C.c_void_p,C.c_uint]
def status():
 s=(C.c_uint*10)();c.sh_coop_status(s);return list(s)
def save():
 b=C.create_string_buffer(c.sh_coop_state_size());assert c.sh_coop_save(b,len(b));return p.save(),b.raw
def restore(s):
 p.restore(s[0]);b=C.create_string_buffer(s[1]);assert c.sh_coop_restore(b,len(s[1]))
p.restore((ROOT/'diagnostics/room.state').read_bytes());c.sh_coop_enable(1);p.run(2);base=save()
for who in (0,1):
 restore(base);p.buttons[1]={7} if who else set();p.run(12,{7} if not who else set());s=status();print('move',who,s)
 assert s[2+2*who]>(342 if who==0 else 374)
 assert s[2+2*(1-who)]==(374 if who==0 else 342)
restore(base);p.buttons[1]=set();p.run(130);s=status();assert s[7]<252;print('damage',s)
mid=save()
def replay():
 p.buttons[1]={0,7};p.run(60,(0,));return status(),hashlib.sha256(p.frame[0]).hexdigest(),hashlib.sha256(p.ram()).hexdigest()
a=replay();restore(mid);b=replay();assert a==b,(a,b);print('deterministic replay passed')
restore(base);p.buttons[1]={0};p.run(90);p.screenshot(ROOT/'diagnostics/coop-combat.png');print('combat',status())

