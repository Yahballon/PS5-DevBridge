from pathlib import Path
import json,socket,struct,argparse,hashlib
ROOT=Path(__file__).resolve().parents[1]
def request(op,data=b'',tail=b'',timeout=45):
 c=json.loads((ROOT/'local-config.json').read_text())
 with socket.create_connection((c['host'],c['port']),5) as s:
  s.settimeout(timeout);s.sendall(struct.pack('<8s64sII',b'PS5DEV1\0',c['key'].encode(),op,len(data))+data+tail)
  out=bytearray()
  while len(out)<34*1024*1024:
   try:b=s.recv(65536)
   except socket.timeout:break
   if not b:break
   out.extend(b)
  return bytes(out)
def deploy(path,tail=b''):
 data=Path(path).read_bytes()
 if not data.startswith(b'\x7fELF') or not 64<=len(data)<=8*1024*1024:raise ValueError('Expected ELF up to 8 MiB')
 print('(AI-assisted) Sending',Path(path).name,'sha256',hashlib.sha256(data).hexdigest()[:12])
 out=request(2,data,tail)
 if b'JBSHOT1 ' not in out and b'JBLOG1 BEGIN' not in out:
  visible=out.decode(errors='replace')[-8000:]
  if visible: request(7,(Path(path).name+' output:\n'+visible).encode()[:8192])
 return out
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('command',choices=['status','logs','deploy','browser','open-browser','stop']);p.add_argument('value',nargs='?');a=p.parse_args()
 if a.command=='deploy':out=deploy(a.value)
 elif a.command=='browser':out=request(4,a.value.encode())
 elif a.command=='open-browser':out=request(9,a.value.encode())
 elif a.command=='stop':out=request(8)
 else:out=request(1 if a.command=='status' else 3)
 print(out.decode(errors='replace'))
