"""(AI-assisted) Start package installation, query progress, or uninstall a named title."""
import argparse,struct,re
from bridge import request
def command(op,value):
 b=value.encode()
 if len(b)>1023 or b'\0' in b:raise ValueError('Invalid value')
 if op==15 and (not re.fullmatch(r'[A-Z]{4}[0-9]{5}',value) or value.startswith('NPXS')):raise ValueError('Expected non-system title ID')
 out=request(op,struct.pack('<I1024s',op,b),timeout=45)
 if not out:raise RuntimeError('Package command failed without response')
 return out.decode(errors='replace')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('command',choices=['install','uninstall','status']);p.add_argument('value');a=p.parse_args();print(command({'install':14,'uninstall':15,'status':19}[a.command],a.value))
