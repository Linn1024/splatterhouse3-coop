from probe import *
p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll'); c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
d=(ROOT/'diagnostics/user-save/Core.bin').read_bytes();p.restore(d[4:4+int.from_bytes(d[:4],'little')]);p.run(5)
c.sh_probe_player.restype=C.POINTER(C.c_ubyte)
c.retro_get_memory_data.restype=C.c_void_p; p.ram_ptr=c.retro_get_memory_data(2); r=p.ram()
print('level',r[0x166], 'pause',r[0x18:0x28].hex())
for i in range(1,64):
 a=0x551e+2*i
 if int.from_bytes(r[a:a+2],'big')&0x8000:
  print(i, [(hex(o),int.from_bytes(r[a+o:a+o+2],'big')) for o in (0,0x80,0x280,0x300,0x380,0x1080)],int.from_bytes(r[0x5f9e+4*i:0x5fa0+4*i],'big'),int.from_bytes(r[0x619e+4*i:0x61a0+4*i],'big'))
p.screenshot(ROOT/'diagnostics/user-save-updated.png')

s=(C.c_uint*10)();c.sh_coop_status(s);print('before',list(s));p.run(120);c.sh_coop_status(s);print('after',list(s));print('ram0',p.ram()[:24].hex());p.screenshot(ROOT/'diagnostics/user-save-after.png')
print('menu',hex(c.sh_coop_menu_get()),'cheats',c.sh_coop_debug_flags()); c.sh_coop_menu_set(0)
p.run(1,(3,));p.run(1);p.run(120);c.sh_coop_status(s);print('resume',list(s));p.screenshot(ROOT/'diagnostics/user-save-resumed.png')
p.run(1,(3,));p.run(1)
c.al_probe_start();p.run(1);ptr=C.POINTER(C.c_uint)();n=c.al_probe_stop(C.byref(ptr));from collections import Counter
print(Counter(ptr[:n]).most_common(25));print([hex(x) for x in ptr[:n] if 0x2a00<x<0x4000][:70])
