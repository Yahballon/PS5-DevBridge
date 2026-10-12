"""(AI-assisted) Install private pairing files locally for the public native app."""
import json,tempfile
from pathlib import Path
from bridge import ROOT
from files import upload
if __name__=='__main__':
 cfg=json.loads((ROOT/'local-config.json').read_text())
 with tempfile.TemporaryDirectory() as folder:
  key=Path(folder)/'pairing-key';key.write_bytes(cfg['key'].encode())
  upload(key,'/data/ps5-devbridge/pairing-key',overwrite=True)
 upload(ROOT/'dist/devbridge-loader.elf','/data/ps5-devbridge/devbridge-loader.elf',overwrite=True)
 print('(AI-assisted) Local pairing and payload prepared; no key printed')
