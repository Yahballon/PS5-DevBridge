"""(AI-assisted) Configure the PS5 address; LAN mode needs no pairing key."""
import argparse,json,ipaddress
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--host',required=True);p.add_argument('--peer');p.add_argument('--browser-home');a=p.parse_args()
ip=ipaddress.ip_address(a.host)
if ip.version!=4 or not (ip in ipaddress.ip_network('10.0.0.0/8') or ip in ipaddress.ip_network('172.16.0.0/12') or ip in ipaddress.ip_network('192.168.0.0/16')):raise ValueError('Use a private LAN IPv4 address')
(ROOT/'local-config.json').write_text(json.dumps({'host':a.host,'port':1337},indent=2)+'\n')
print('(AI-assisted) PS5 address configured; no key or rebuild needed')
