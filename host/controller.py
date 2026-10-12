"""(AI-assisted) Bounded firmware-5.10 controller commands via port 1337."""
import argparse,struct
from bridge import request
BUTTONS={'create':1,'l3':2,'r3':4,'options':8,'up':16,'right':32,'down':64,'left':128,'l2':256,'r2':512,'l1':1024,'r1':2048,'triangle':4096,'circle':8192,'cross':16384,'square':32768,'touchpad':1048576,'neutral':0}
def encode(steps):
 if not 1<=len(steps)<=32:raise ValueError('1..32 steps required')
 total=0;data=bytearray(struct.pack('<III',0x50414431,1,len(steps)))
 for step in steps:
  mask=step.get('buttons',0);duration=step.get('duration',100);pause=step.get('pause',300)
  if mask&~0x0010ffff or not 16<=duration<=3000 or not 0<=pause<=3000:raise ValueError('Invalid buttons or timing')
  analog=[step.get(k,128 if i<4 else 0) for i,k in enumerate(['lx','ly','rx','ry','l2','r2'])]
  if any(not 0<=n<=255 for n in analog):raise ValueError('Analog values must be 0..255')
  total+=duration+pause;data+=struct.pack('<IHH8B',mask,duration,pause,*analog,0,0)
 if total>15000:raise ValueError('Sequence limit is 15 seconds')
 return bytes(data)
def run(steps):return request(10,encode(steps),timeout=40)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('button',choices=BUTTONS);p.add_argument('--ms',type=int,default=100);p.add_argument('--lx',type=int,default=128);p.add_argument('--ly',type=int,default=128);p.add_argument('--rx',type=int,default=128);p.add_argument('--ry',type=int,default=128);p.add_argument('--l2',type=int,default=0);p.add_argument('--r2',type=int,default=0);a=p.parse_args()
 print(run([dict(buttons=BUTTONS[a.button],duration=a.ms,lx=a.lx,ly=a.ly,rx=a.rx,ry=a.ry,l2=a.l2,r2=a.r2)]).decode(errors='replace'))
