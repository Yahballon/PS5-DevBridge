"""(AI-assisted) Generate private build pairing inputs; never publish local-config.json."""
import argparse,json,secrets,ipaddress
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--host',required=True);p.add_argument('--peer',required=True);p.add_argument('--browser-home',default='https://example.com/');a=p.parse_args()
for value in [a.host,a.peer]:
 ip=ipaddress.ip_address(value)
 if ip.version!=4 or not ip.is_private:raise ValueError('Use local IPv4 addresses')
if not a.browser_home.startswith(('http://','https://')) or any(c in a.browser_home for c in ['"','\\','\n','\r']):raise ValueError('Invalid browser URL')
key=secrets.token_hex(32)
(ROOT/'local-config.json').write_text(json.dumps(dict(host=a.host,peer=a.peer,port=1337,key=key),indent=2))
(ROOT/'loader/bridge_config.h').write_text(f'#pragma once\n#define BRIDGE_KEY "{key}"\n#define BRIDGE_PEER "{a.peer}"\n#define BRIDGE_PORT 1337\n')
(ROOT/'app/src/local_config.hpp').write_text(f'#pragma once\n#define BRIDGE_KEY "{key}"\n#define BROWSER_HOME "{a.browser_home}"\n')
print('(AI-assisted) Private pairing configured; rebuild the payload (and optional app)')
