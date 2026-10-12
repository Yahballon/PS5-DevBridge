"""(AI-assisted) Verify homebrew-only management framing and UI evidence semantics."""
import sys,struct,tempfile,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'host'))
import apps,ui_test
calls=[]
apps.request=lambda op,data,timeout: calls.append((op,data)) or b'APP REGISTERED\n'
apps.command('register','PPSA99339','/mnt/usb0/homebrew/PPSA99339')
assert calls[0][0]==20 and len(calls[0][1])==1044
assert struct.unpack('<I',calls[0][1][:4])[0]==1
for title in ['NPXS40087','../bad','PPSA9933']:
 try:apps.command('remove',title)
 except ValueError:pass
 else:raise AssertionError('invalid title accepted')
from PIL import Image
with tempfile.TemporaryDirectory() as td:
 root=Path(td);shot=root/'shot.png';baseline=root/'baseline.png';Image.new('RGB',(4,4),'black').save(shot);Image.new('RGB',(4,4),'white').save(baseline)
 ui_test.capture=lambda:shot;ui_test.request=lambda op:b'LOG';ui_test.time.sleep=lambda n:None
 manifest=root/'checks.json';manifest.write_text(json.dumps({'checks':[{'name':'Evidence only'}]}));result=ui_test.test(manifest);assert result[0]['passed'] is None
 manifest.write_text(json.dumps({'checks':[{'name':'Mismatch','baseline':'baseline.png','max_mean_error':1}]}))
 try:ui_test.test(manifest)
 except RuntimeError:pass
 else:raise AssertionError('mismatch passed')
print('PASS managed title bounds, protocol framing, no invented visual pass, baseline mismatch')
