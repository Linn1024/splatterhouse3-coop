exec(compile(open(__file__.replace('inspect_sprite_sat.py','explore_sprite_save.py')).read().split('for t in range(60):')[0],__file__,'exec'))
c.al_probe_vram.restype=C.POINTER(C.c_ubyte)
for t in range(4):
 p.run(1);r=p.ram();v=bytes(c.al_probe_vram()[:65536]);v=bytes(x for i in range(0,len(v),2) for x in (v[i+1],v[i]))
 x=C.create_string_buffer(c.sh_coop_state_size());c.sh_coop_save(x,len(x));ex=x.raw
 print('FRAME',t)
 for i in range(25):
  a=0xa000+8*i # actual SAT base reg5?
  c.sh_probe_vdp_regs.restype=C.POINTER(C.c_ubyte);a=((c.sh_probe_vdp_regs()[5]&0x7f)<<9)+8*i
  q=v[a:a+8];attr=int.from_bytes(q[4:6],'big');flag=ex[144+0x550+i]
  if flag:print(i,flag,'tile',attr&2047,'size',hex(q[2]),'xy',int.from_bytes(q[6:8],'big'),int.from_bytes(q[:2],'big'))
