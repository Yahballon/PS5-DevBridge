"""(AI-assisted) Reproducible input/screenshot checks with explicit visual baselines."""
import argparse,json,time,shutil
from pathlib import Path
from controller import run
from screenshot import capture
from bridge import request
def test(manifest):
 p=Path(manifest).resolve();cfg=json.loads(p.read_text());out=p.parent/'devbridge-results';out.mkdir(exist_ok=True);results=[]
 for index,step in enumerate(cfg['checks']):
  inputs=step.get('steps',[])
  if inputs:
   reply=run(inputs)
   if b'INPUT RESULT 0' not in reply:raise RuntimeError(reply.decode(errors='replace'))
  time.sleep(min(10,max(0,step.get('settle_seconds',1))))
  shot=capture();saved=out/f'{index:02d}.png';shutil.copyfile(shot,saved);result={'name':step['name'],'screenshot':str(saved),'passed':None}
  if 'baseline' in step:
   from PIL import Image,ImageChops,ImageStat
   a=Image.open(saved).convert('RGB');b=Image.open(p.parent/step['baseline']).convert('RGB')
   if a.size!=b.size:raise RuntimeError('Baseline dimensions differ')
   if 'region' in step:a=a.crop(tuple(step['region']));b=b.crop(tuple(step['region']))
   error=sum(ImageStat.Stat(ImageChops.difference(a,b)).mean)/3;result.update(mean_pixel_error=error,passed=error<=step.get('max_mean_error',4))
  results.append(result)
 (out/'ui-results.json').write_text(json.dumps(results,indent=2));(out/'events.log').write_bytes(request(3))
 if any(r['passed'] is False for r in results):raise RuntimeError('Visual baseline mismatch; inspect ui-results.json')
 return results
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('manifest');print(test(p.parse_args().manifest))
