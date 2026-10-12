"""(AI-assisted) File client integration against an in-memory protocol peer."""
import sys,struct,tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'host'))
import files,packages
storage={};corrupt=False
def peer(op,data,**kwargs):
 path,offset,length,flags=struct.unpack('<1024sQII',data[:1040]);path=path.split(b'\0')[0].decode()
 if op==12:
  stage=path+'.devbridge-part'
  if flags:storage[stage]=bytearray()
  buf=storage[stage];buf[offset:offset+length]=data[1040:];return b'FILE OK\n'
 if op==11:
  buf=storage[path];chunk=bytes(buf[offset:offset+length])
  if corrupt and chunk:chunk=b'X'+chunk[1:]
  return f'FILE DATA {len(chunk)} {len(buf)}\n'.encode()+chunk
 if op==13:
  if path in storage and not flags:return b'FILE ERROR commit refused\n'
  storage[path]=storage.pop(path+'.devbridge-part');return b'FILE OK\n'
 raise AssertionError(op)
files.request=peer
with tempfile.TemporaryDirectory() as td:
 src=Path(td)/'source';dst=Path(td)/'dest';content=bytes(range(256))*9000;src.write_bytes(content)
 assert files.upload(src,'/data/ps5-devbridge/test')==len(content)
 files.download('/data/ps5-devbridge/test',dst);assert dst.read_bytes()==content
 try:files.upload(src,'/data/ps5-devbridge/test')
 except RuntimeError:pass
 else:raise AssertionError('overwrite unexpectedly accepted')
 corrupt=True
 try:files.upload(src,'/data/ps5-devbridge/bad')
 except RuntimeError:pass
 else:raise AssertionError('corruption unexpectedly committed')
 assert '/data/ps5-devbridge/bad' not in storage
for path in ['/system/test','/data/a/../b','/mnt/usb8/a']:
 try:files.frame(path)
 except ValueError:pass
 else:raise AssertionError('invalid path accepted')
try:packages.command(15,'NPXS40087')
except ValueError:pass
else:raise AssertionError('system title accepted')
print('PASS multi-chunk roundtrip, overwrite protection, corrupted staging rejection, path/title bounds')
