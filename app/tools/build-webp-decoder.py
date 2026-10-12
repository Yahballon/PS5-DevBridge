# (AI-assisted) Build only the official libwebp decoder sources, without worker threads.
from pathlib import Path
import os,re,subprocess
root=Path(__file__).resolve().parent.parent
source=root/'3rdparty/libwebp';host=os.environ.get('WEBP_BUILD_HOST')=='1';out=root/('build/webp-host' if host else 'build/webp');out.mkdir(parents=True,exist_ok=True)
paths=list((source/'src/dec').glob('*.c'))
for directory in ('dsp','utils'):
 text=(source/'src'/directory/'Makefile.am').read_text()
 for name in re.findall(r'^COMMON_SOURCES \+= ([A-Za-z0-9_]+\.c)$',text,re.M):paths.append(source/'src'/directory/name)
 if directory=='dsp':
  for name in re.findall(r'^libwebpdspdecode_(?:sse2|sse41|avx2)_la_SOURCES \+= ([A-Za-z0-9_]+\.c)$',text,re.M):paths.append(source/'src/dsp'/name)
header_time=max(p.stat().st_mtime for p in source.rglob('*.h'))
env=dict(os.environ,PS5_PAYLOAD_SDK=str(root/'.deps/native/ps5-payload-sdk'),PS5_CLANG='/usr/bin/clang-18',USE_CCACHE='0')
objects=[]
for path in paths:
 obj=out/(str(path.relative_to(source)).replace('/','_')+'.o');objects.append(obj)
 if obj.exists() and obj.stat().st_mtime>=max(header_time,path.stat().st_mtime):continue
 subprocess.run([('/usr/bin/clang-18' if host else str(root/'tooling/prospero-clang18')),'-std=c11','-O2','-fPIC','-ffunction-sections','-fdata-sections','-I'+str(source),'-c',str(path),'-o',str(obj)],env=env,check=True)
archive=out/'libwebpdecoder.a'
if not archive.exists() or any(o.stat().st_mtime>archive.stat().st_mtime for o in objects):subprocess.run(['llvm-ar-18','rcs',str(archive),*(str(o) for o in objects)],check=True)
print('WebP decoder archive ready (AI-assisted).')
