"""(AI-assisted) Manage explicitly registered native homebrew only."""
import argparse,struct,re,json,urllib.request
from pathlib import Path
from files import upload
from bridge import request
ACTIONS={'list':0,'register':1,'start':2,'close':3,'update':4,'remove':5,'state':6,'stage':7,'mkdir':8,'close-any':9,'close-current':10}
def command(action,title='',path=''):
 if action not in ['list','close-current'] and (not re.fullmatch(r'[A-Z]{4}[0-9]{5}',title) or title.startswith('NPXS')):raise ValueError('Expected homebrew title ID')
 if len(path.encode())>1023:raise ValueError('Path too long')
 out=request(20,struct.pack('<I16s1024s',ACTIONS[action],title.encode(),path.encode()),timeout=45)
 if not out:raise RuntimeError('Managed-app command refused or failed; check registered path/title')
 return out.decode(errors='replace')
def install_folder(folder,title,remote):
 root=Path(folder).resolve();metadata=json.loads((root/'sce_sys/param.json').read_text())
 if metadata['titleId']!=title:raise ValueError('Folder title mismatch')
 try:
  state=command('state',title)
  if 'running=1' in state:raise RuntimeError('Close the app before updating')
 except RuntimeError as e:
  if 'running' in str(e):raise
 if 'APP STAGE READY' not in command('stage',title,remote):raise RuntimeError('Stage preparation failed')
 stage=remote+'.devbridge-stage'
 for item in sorted(root.rglob('*'),key=lambda p:(len(p.relative_to(root).parts),str(p))):
  if item.is_symlink():raise ValueError('Symlinks are not installable')
  rel=item.relative_to(root).as_posix()
  if item.is_dir():
   if 'result=0' not in command('mkdir',title,stage+'/'+rel):raise RuntimeError('Directory creation failed')
  elif item.is_file():upload(item,stage+'/'+rel)
 # ShadowMount must release the existing folder before atomic promotion.
 from bridge import ROOT
 c=json.loads((ROOT/'local-config.json').read_text());url=f"http://{c['host']}:10101/api/v1/games/unmount"
 try:
  response=urllib.request.urlopen(urllib.request.Request(url,data=json.dumps({'title_id':title}).encode(),headers={'Content-Type':'application/json'}),timeout=20)
  if json.loads(response.read()).get('status') not in [0]:raise RuntimeError('Unmount refused')
 except urllib.error.HTTPError as e:
  if e.code!=404:raise
 result=command('update',title)
 if 'APP UPDATED' not in result:raise RuntimeError(result)
 for endpoint,body in [('manual/add',{'path':remote}),('games/mount',{'title_id':title})]:
  url=f"http://{c['host']}:10101/api/v1/{endpoint}"
  response=urllib.request.urlopen(urllib.request.Request(url,data=json.dumps(body).encode(),headers={'Content-Type':'application/json'}),timeout=20)
  if json.loads(response.read()).get('status')!=0:raise RuntimeError('Mount/registration failed')
 return result
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('action',choices=[*ACTIONS,'install-folder']);p.add_argument('title',nargs='?',default='');p.add_argument('path',nargs='?',default='');a=p.parse_args();print(install_folder(a.path,a.title,'/mnt/usb0/homebrew/'+a.title) if a.action=='install-folder' else command(a.action,a.title,a.path))
