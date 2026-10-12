"""(AI-assisted) Create a separate native homebrew project from the tested UI starter."""
import argparse,shutil,json,re
from pathlib import Path
from bridge import ROOT
def create(destination,title,name):
 if not re.fullmatch(r'[A-Z]{4}[0-9]{5}',title) or title.startswith('NPXS'):raise ValueError('Use a non-system title ID')
 if not re.fullmatch(r'[A-Za-z0-9 _-]{1,48}',name):raise ValueError('Use a short ASCII app name')
 dest=Path(destination)
 if dest.exists():raise ValueError('Destination must be a new directory')
 shutil.copytree(ROOT/'templates/simple-calc',dest,ignore=shutil.ignore_patterns('.deps','build','dist','*.log','__pycache__'))
 param=dest/'sce_sys/param.json';c=json.loads(param.read_text());c['titleId']=title;c['conceptId']=title[4:];c['contentId']=f'UP9000-{title}_00-DEVBRIDGEAPP0001';c['localizedParameters']['en-US']['titleName']=name;param.write_text(json.dumps(c,indent=2)+'\n')
 source=dest/'src/main.cpp';source.write_text(source.read_text().replace('SIMPLE CALC',name.upper()).replace('PPSA99339',title))
 return dest
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('destination');p.add_argument('--title',required=True);p.add_argument('--name',required=True);a=p.parse_args();print(create(a.destination,a.title,a.name))
