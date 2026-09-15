"""The reported QuickSave1 renders independently of the shared static Rick tiles."""
from probe import *
import zipfile,struct
source=Path(sys.argv[1]) if len(sys.argv)>1 else Path(r'C:/TEMP2/Bizhawk/Libretro/State/splatterhouse_coop_libretro/Splatterhouse 3 (USA).QuickSave1.State')
original_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with zipfile.ZipFile(source) as archive:d=archive.read('Core.bin')[4:]
h=struct.unpack('<8I',d[:32]);native=d[32:32+h[2]];extra=d[32+h[2]:32+h[2]+h[3]]
p=Probe();c=p.core;c.al_probe_vram.restype=C.POINTER(C.c_ubyte)
def restore(n=native,e=extra):
 p.restore(n);b=C.create_string_buffer(e);assert c.sh_coop_restore(b,len(e))
def snapshot():
 b=C.create_string_buffer(c.sh_coop_state_size());assert c.sh_coop_save(b,len(b));return p.save(),b.raw
restore();vram_offset=16+65536+8192+1+4+16+0x400
assert native[vram_offset:vram_offset+65536]==bytes(c.al_probe_vram()[:65536])
rom=p.rom_path.read_bytes();frames=[]
for label,art in [('saved',None),('normal',rom[0xac000:0xac7e0]),('powered',rom[0xac7e0:0xad0e0]),('empty',bytes(72*32))]:
 n=bytearray(native)
 if art is not None:
  swapped=bytearray(art);swapped[0::2]=art[1::2];swapped[1::2]=art[0::2]
  n[vram_offset+96*32:vram_offset+96*32+len(art)]=swapped
 restore(bytes(n));p.run(1);frames.append(p.frame[0])
 assert frames[-1]==frames[0],('Global static art leaked into Rick sprites',label)
restore();p.run(1);p.screenshot(ROOT/'diagnostics/quicksave1-sprites-fixed.png');seed=snapshot()
def replay():
 p.buttons[1]={0};p.run(120,(0,));return hashlib.sha256(p.frame[0]).hexdigest(),p.ram()
a=replay();restore(*seed);assert replay()==a
assert hashlib.sha256(source.read_bytes()).hexdigest()==original_hash
print('PASS: QuickSave1 first rendered frame ignores mismatched/empty shared art, combat save replay passes, original unchanged')
