"""Stage immutable native Rick art in a headless probe, without advancing CPU."""
from probe import C,ROOT

def native_pose(frame):
 rom=(ROOT/'Splatterhouse 3 (USA).md').read_bytes()
 def u32(a):return int.from_bytes(rom[a:a+4],'big')
 def signed(n):return n-256 if n&128 else n
 table=u32((0xe0000+(frame-125)*4) if frame>=125 else (0xc0000+frame*4))
 # Four native 384-byte DMA chunks, each preceded by a two-byte command.
 art=b''.join(rom[u32(table+k*4)+2:u32(table+k*4)+386] for k in range(4))
 assert len(art)==1536
 ptr=u32(0x100000+frame*4);pieces=[]
 for _ in range(20):
  count=rom[ptr];ptr+=1
  if not count:return pieces,art
  x,y=signed(rom[ptr]),signed(rom[ptr+1]);ptr+=2
  for _ in range(count&127):
   size=rom[ptr];attr=int.from_bytes(rom[ptr+1:ptr+3],'big');ptr+=3
   pieces.append((x,y,size,attr))
   if count&128:y+=8+((size&3)<<3)
   else:x+=8+((size&12)<<1)
 raise AssertionError('Unterminated native sprite layout')

def stage_pose(core,frame):
 pieces,art=native_pose(frame)
 core.al_probe_vram.restype=C.POINTER(C.c_ubyte)
 core.sh_probe_vdp_regs.restype=C.POINTER(C.c_ubyte)
 core.sh_coop_save.argtypes=core.sh_coop_restore.argtypes=[C.c_void_p,C.c_uint]
 buf=C.create_string_buffer(core.sh_coop_state_size())
 assert core.sh_coop_save(buf,len(buf));extra=bytearray(buf.raw)
 # Either dynamic tile bank can be named by the native descriptor. Populate
 # both with this completed pose, in the core's word-swapped representation.
 bank=bytearray(art);bank[::2]=art[1::2];bank[1::2]=art[::2]
 extra[-0xc00:]=bank*2
 extra[144+0x550:144+0x5a0]=bytes([13 if frame>=125 else 9])*len(pieces)+bytes(80-len(pieces))
 extra[144+65536+0x57a0:144+65536+0x57a2]=frame.to_bytes(2,'big')
 buf=C.create_string_buffer(bytes(extra));assert core.sh_coop_restore(buf,len(extra))
 v=core.al_probe_vram();base=(core.sh_probe_vdp_regs()[5]&127)<<9
 def word(a,value):v[a^1]=(value>>8)&255;v[(a+1)^1]=value&255
 for i,(x,y,size,attr) in enumerate(pieces):
  a=base+i*8;word(a,y+256);v[(a+2)^1]=size;v[(a+3)^1]=i+1 if i+1<len(pieces) else 0
  word(a+4,(attr&~0x6000)|0x2000);word(a+6,x+256)
 return pieces
