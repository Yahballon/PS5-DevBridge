#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
root=$PWD
mkdir -p build/unrar
# Keep vendored sources unmodified and compile a single-threaded decompressor.
python3 - "$root" <<'PY'
from pathlib import Path
import re,sys,subprocess,os
r=Path(sys.argv[1]);text=(r/'third_party/unrar/makefile').read_text()
names=re.search(r'OBJECTS=(.*?)\n\n',text,re.S).group(1).replace('\\','').split()+['filestr.o','scantree.o','dll.o','qopen.o']
env=os.environ.copy();env['PS5_PAYLOAD_SDK']=str(r/'.deps/native/ps5-payload-sdk');env['PS5_CLANG']='/usr/bin/clang-18'
objects=[]
for name in names:
 source=r/'third_party/unrar'/name.replace('.o','.cpp');obj=r/'build/unrar'/name;objects.append(str(obj))
 if not obj.exists() or source.stat().st_mtime>obj.stat().st_mtime:
  subprocess.run(['sh',str(r/'tooling/prospero-clang18'),'-std=c++11','-O2','-fPIC','-fexceptions','-fcxx-exceptions','-D_UNIX','-DRARDLL','-D_FILE_OFFSET_BITS=64','-D_LARGEFILE_SOURCE','-c',str(source),'-o',str(obj)],env=env,check=True)
subprocess.run(['/usr/bin/llvm-ar-18','rcs',str(r/'build/unrar/libunrar.a'),*objects],check=True)
PY
