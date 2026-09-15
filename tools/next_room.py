from verify_exit import *
for i in range(20):
 p.buttons[1]=set();p.run(90);p.run(1,(3,));print('skip',i,status(),p.ram()[:4].hex());
 if status()[0]:break
p.screenshot(ROOT/'diagnostics/next-room.png')
