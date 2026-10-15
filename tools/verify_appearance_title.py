"""Fourth native title entry, selection/shadows, and preserved skin highlights."""
from probe import *

p=Probe(ROOT/'bizhawk/splatterhouse_coop_libretro.dll')
c=C.CDLL(str(ROOT/'bizhawk/splatterhouse_engine.dll'))
c.retro_get_memory_data.restype=C.c_void_p;p.ram_ptr=c.retro_get_memory_data(2)

def pulse(buttons=(),who=0):
 p.buttons[1]=set(buttons) if who else set()
 p.run(1,buttons if not who else ())
 p.buttons[1]=set();p.run(2)

p.run(600);pulse((3,));p.run(180);pulse((3,));p.run(60)
assert p.ram()[1:3]==bytes([1,3])
title=p.save()
p.screenshot(ROOT/'diagnostics/appearance-title-fourth.png')
for expected in (1,2,3,3):
 pulse((5,),1);assert int.from_bytes(p.ram()[0x64:0x66],'big')==expected
p.screenshot(ROOT/'diagnostics/appearance-title-selected.png')
assert c.sh_coop_appearance_available()&4
pulse((3,),1);assert c.sh_coop_appearance_menu_get()&1
assert p.ram()[1:3]==bytes([1,3])

def rgb(x,y):
 data,w,h,pitch=p.frame
 return int.from_bytes(data[y*pitch+x*4:y*pitch+x*4+4],'little')&0xffffff

names=('CLOTHES','MASK','SKIN','BACK')
def check_selection(selected):
 for row,name in enumerate(names):
  color=0xeeeecc if row==selected else 0xcdce00
  foreground=shadow=0
  for i,ch in enumerate(name):
   for y in range(8):
    ink=c.sh_coop_font_row(ord(ch),y)
    dark=c.sh_coop_font_row(ord(ch),y|8)
    for x in range(8):
     if ink&(128>>x):assert rgb(32+i*8+x,64+row*24+y)==color;foreground+=1
     elif dark&(128>>x):assert rgb(32+i*8+x,64+row*24+y)==0x620000;shadow+=1
  assert foreground>20 and shadow>10

for row in range(4):
 check_selection(row)
 if row<3:pulse((5,),row%2)
pulse((8,),1);assert not c.sh_coop_appearance_menu_get()&9
assert p.ram()[1:3]==bytes([1,3]) and p.ram()[0x65]==3
# Return keeps the main-menu cursor on COOP OPTIONS and consumes held Start.
p.run(1,(3,));assert c.sh_coop_appearance_menu_get()&1
p.run(3);p.run(12,(3,));p.run(3)
assert not c.sh_coop_appearance_menu_get()&9 and p.ram()[1:3]==bytes([1,3])
# Original Options and Password still route to their native screens.
p.restore(title);pulse((5,));pulse((3,));p.run(120)
assert p.ram()[1:3]==bytes([8,3])
p.restore(title);pulse((5,));pulse((5,));pulse((3,));p.run(120)
assert p.ram()[1]==15
# Purple skin keeps the same pale specular highlights as the original.
p.restore(title)
for _ in range(3):pulse((5,))
pulse((3,));c.sh_coop_appearance_set(0x060001);p.run(1)
p.screenshot(ROOT/'diagnostics/appearance-purple-fixed.png')
highlights=[]
for y in range(72):
 for x in range(33):
  material=c.sh_coop_preview_ink(x,y)
  if material in (18,19):highlights.append((x,y,material))
assert len(highlights)>15
for color in range(1,10):
 c.sh_coop_appearance_set(color<<16);p.run(1)
 for x,y,material in highlights:
  expected=0xeeeecc if material==18 else 0xccccaa
  assert rgb(224+x*2,62+y*2)==expected
# Mask colors must never recolor the neighboring skin glints reported by user.
skin_glints=((15,3),(13,7),(13,8),(14,12),(15,16),(19,16),(14,17),(16,17))
for color in range(1,10):
 c.sh_coop_appearance_set(1|(color<<8));p.run(1)
 for x,y in skin_glints:assert rgb(224+x*2,62+y*2)==0xeeeecc,(color,x,y)
c.sh_coop_appearance_set(1|(6<<8));p.run(1)
p.screenshot(ROOT/'diagnostics/appearance-mask-purple-fixed.png')
print('PASS fourth native title entry, P2 navigation, original Options/Password, clear selected rows, native shadows, return handling and pale skin highlights across every color')
