"""(AI-assisted) Chunked transfer with staged commits and read-back verification."""
import argparse,struct,re,hashlib
from pathlib import Path
from bridge import request
CHUNK=1024*1024
def frame(path,offset=0,length=0,flags=0):
 b=path.encode()
 if len(b)>1023 or b'\0' in b or '..' in path.split('/') or not (path.startswith('/data/') or re.match(r'^/mnt/usb[0-7]/',path)):raise ValueError('Use a /data/ or /mnt/usb0..7/ path without traversal')
 return struct.pack('<1024sQII',b,offset,length,flags)
def chunk(path,offset):
 out=request(11,frame(path,offset,CHUNK));m=re.match(rb'FILE DATA (\d+) (\d+)\n',out)
 if not m:raise RuntimeError(out.decode(errors='replace') or 'File read failed')
 n,total=map(int,m.groups());data=out[m.end():]
 if len(data)!=n:raise RuntimeError('Incomplete file chunk')
 return data,total
def download(remote,local):
 p=Path(local);stage=p.with_name(p.name+'.part');offset=0
 with stage.open('wb') as f:
  while True:
   b,total=chunk(remote,offset);f.write(b);offset+=len(b)
   if offset==total:break
   if not b:raise RuntimeError('Unexpected empty chunk')
 stage.replace(p);return p
def upload(local,remote,overwrite=False):
 p=Path(local);offset=0;digest=hashlib.sha256()
 with p.open('rb') as f:
  first=True
  while True:
   b=f.read(CHUNK)
   if not b and not first:break
   out=request(12,frame(remote,offset,len(b),int(first))+b,timeout=45)
   if out!=b'FILE OK\n':raise RuntimeError(out.decode(errors='replace'))
   first=False;offset+=len(b);digest.update(b)
   if not b:break
 # Verify staging content before committing the final name.
 check=hashlib.sha256();at=0
 while at<offset:
  b,total=chunk(remote+'.devbridge-part',at)
  if total!=offset or not b:raise RuntimeError('Staging size mismatch')
  check.update(b);at+=len(b)
 if check.digest()!=digest.digest():raise RuntimeError('Staging SHA256 mismatch')
 out=request(13,frame(remote,offset,flags=int(overwrite)))
 if out!=b'FILE OK\n':raise RuntimeError(out.decode(errors='replace'))
 return offset
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('command',choices=['put','get']);p.add_argument('source');p.add_argument('destination');p.add_argument('--overwrite',action='store_true');a=p.parse_args()
 print(upload(a.source,a.destination,a.overwrite) if a.command=='put' else download(a.source,a.destination))
