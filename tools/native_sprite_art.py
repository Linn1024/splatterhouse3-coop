from probe import *
import struct
rom=(ROOT/'Splatterhouse 3 (USA).md').read_bytes()
def u16(a):return int.from_bytes(rom[a:a+2],'big')
def u32(a):return int.from_bytes(rom[a:a+4],'big')
def decompress(block):
 a=u32(0x120000+block*4);out=bytearray()
 for _ in range(256):
  bits=u16(a);a+=2
  if bits&0x8000:
   words=[rom[a:a+2],rom[a+2:a+4]];a+=4
   for bit in range(13,-1,-1):
    if bits&(1<<bit):words.append(words[-2])
    else:words.append(rom[a:a+2]);a+=2
   out.extend(b''.join(words))
  else:out.extend(rom[a:a+32]);a+=32
 return out
cache={}
def gfx(sheet):
 a=u32(0x55b1c+sheet*4);dest=u16(a)*32;a+=2;writes=[]
 for _ in range(2048):
  block=rom[a];a+=1
  if block==255:break
  if block==253:dest=u16(a)*32;a+=2;continue
  if block==254:data=bytes(32)
  else:
   start=rom[a]*32;count=rom[a+1]*32;a+=2
   if block not in cache:cache[block]=decompress(block)
   data=cache[block][start:start+count]
  writes.append((dest,data));dest+=len(data)
 return writes
