"""Real back/forward/back+Attack, both Ricks, with native ROM row comparison."""
__file__=__file__.replace('verify_gut_blast.py','verify_mixed_forms.py')
exec(compile(open(__file__).read().split('c.al_probe_vram')[0],__file__,'exec'))
c.sh_sprite_row.argtypes=[C.c_uint,C.c_uint,C.c_uint]
c.sh_sprite_row.restype=C.POINTER(C.c_ubyte)
rom=p.rom_path.read_bytes()
for who,direction in ((0,6),(0,7),(1,6),(1,7)):
 restore(mixed_base);p.buttons[1]=set();c.sh_coop_debug(2)
 def tick(n,buttons=()):
  p.buttons[1]=set(buttons) if who else set()
  p.run(n,() if who else buttons)
 tick(1,(1,));tick(240) # Finish the transformation animation before attacking.
 tick(3,(direction,));tick(3)
 back=direction^1
 for buttons in ((back,),(direction,),(back,0)):tick(2,buttons)
 seen=set();checked=0;checkpoint=None
 for t in range(100):
  tick(1);r=p.ram();e=save()[1][144:];idx=0
  for _ in range(80):
   a=0x525e+idx*8;item=r[a:a+8];flags=e[0x500+idx]
   if flags&16:
    assert bool(flags&3)==bool(who)
    attr=int.from_bytes(item[4:6],'big');tile=attr&2047
    count=((item[2]>>2)+1)*((item[2]&3)+1)
    for name in range(tile,tile+count):
     if not 96<=name<205:continue
     seen.add(name)
     for row in range(8):
      address=0xad0e0+(name-96)*32+(7-row if attr&0x1000 else row)*4
      expected=[]
      for x in range(8):
       sx=7-x if attr&0x800 else x
       ink=(rom[address+sx//2]>>(0 if sx&1 else 4))&15
       if who:ink={10:14,11:15,12:15,13:1}.get(ink,ink)
       expected.append(ink)
      actual=c.sh_sprite_row((attr&~2047)|name,row*8,flags)
      assert actual and list(actual[:8])==expected,(who,t,name,row)
      checked+=1
   idx=item[3]&127
   if not idx:break
  if t==25:
   p.screenshot(ROOT/f'diagnostics/gut-blast-fixed-p{who+1}-dir{direction}.png');checkpoint=save()
 assert min(seen)==97 and max(seen)>=204,(who,seen)
 def replay():
  tick(50);return status(),hashlib.sha256(p.frame[0]).hexdigest()
 restore(checkpoint);first=replay();restore(checkpoint);assert replay()==first
 print('PASS P'+str(who+1),'direction',direction,'gut blast:',checked,'native graphics rows, long-sheet tail, correct ownership and saved mid-attack replay')
