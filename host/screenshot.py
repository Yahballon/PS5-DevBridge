"""(AI-assisted) Capture a fresh PS5 screenshot, never return an old image as new."""
import re,struct,time
from pathlib import Path
from bridge import ROOT,request
bridge_request=request
from controller import run

def inventory():
 out=request(16).decode(errors='replace')
 return set(re.findall(r'PHOTO ([^\s]+\.png) bytes=',out))
def capture():
 before=inventory()
 result=run([dict(buttons=1,duration=1440,pause=1000)])
 if b'DEVBRIDGE/1 INPUT RESULT 0' not in result:raise RuntimeError('Controller helper did not confirm successful cleanup')
 for _ in range(5):
  after=inventory();fresh=after-before
  if fresh:break
  time.sleep(1)
 else:raise RuntimeError('No fresh screenshot; no old image was substituted')
 name=sorted(fresh)[-1]
 request=struct.pack('<II768s',0x53484f54,1,name.encode())
 response=bridge_request(17,request)
 match=re.search(rb'JBSHOT1 (\d+)\n',response)
 if not match:raise RuntimeError('Missing PNG response')
 size=int(match[1]);body=response[match.end():match.end()+size]
 if not 8<=size<=32*1024*1024 or len(body)!=size or not body.startswith(b'\x89PNG\r\n\x1a\n'):raise RuntimeError('Incomplete PNG')
 folder=ROOT/'screenshots';folder.mkdir(exist_ok=True);path=folder/Path(name).name;path.write_bytes(body)
 print('(AI-assisted) Fresh screenshot:',path);return path
if __name__=='__main__':capture()
