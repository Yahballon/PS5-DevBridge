"""(AI-assisted) Retrieve reports emitted by opt-in own-app crash reporting."""
import argparse
from files import download
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('remote');p.add_argument('output');a=p.parse_args();path=download(a.remote,a.output);print(path.read_text(errors='replace'))
