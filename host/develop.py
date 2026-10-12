"""(AI-assisted) Explicit project build -> ELF -> input test -> screenshot workflow."""
import argparse,json,subprocess
from pathlib import Path
from bridge import deploy
from controller import run
from screenshot import capture

def develop(manifest):
 path=Path(manifest).resolve();project=path.parent;c=json.loads(path.read_text())
 argv=c['build']
 if not isinstance(argv,list) or not argv or not all(isinstance(v,str) for v in argv):raise ValueError('build must be an argument list')
 print('(AI-assisted) Building',project)
 subprocess.run(argv,cwd=project,check=True,shell=False)
 elf=(project/c['elf']).resolve()
 if not elf.is_relative_to(project):raise ValueError('ELF must stay in the project')
 output=deploy(elf);folder=project/'devbridge-results';folder.mkdir(exist_ok=True);(folder/'stdout.log').write_bytes(output)
 expected=c.get('ready_marker')
 if expected and expected.encode() not in output:raise RuntimeError('App readiness marker missing; deployment is not proof of launch')
 if c.get('steps'):
  result=run(c['steps']);(folder/'controller.log').write_bytes(result)
  if b'DEVBRIDGE/1 INPUT RESULT 0' not in result:raise RuntimeError('Controller sequence did not confirm cleanup')
 if c.get('screenshot',False):return capture()
 print('(AI-assisted) Build and payload output saved; visual verification was not requested')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('manifest');develop(p.parse_args().manifest)
