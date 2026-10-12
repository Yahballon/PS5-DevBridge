from pathlib import Path
import subprocess,shutil,re
r=Path(__file__).resolve().parents[1];out=r/'build/jb-json';out.mkdir(parents=True,exist_ok=True);lib=r/'.deps/pacbrew/v0.40.2/sysroot/user/homebrew/lib/libjson-c.a'
names=set()
for line in subprocess.check_output(['llvm-nm-18','--defined-only',str(lib)],text=True).splitlines():
 parts=line.split()
 if len(parts)>=3 and parts[-2]in {'T','D','B','R','S','W','V'} and re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*',parts[-1]):names.add(parts[-1])
mapping=out/'symbols.map';mapping.write_text(''.join(n+' jb_'+n+'\n'for n in sorted(names)))
header=r/'jb-backend/src/jb_json_symbols.hpp';header.write_text('// (AI-assisted) Isolate JSON-C from the OpenGL shader compiler\'s unrelated Parson API.\n#pragma once\n'+''.join('#define '+n+' jb_'+n+'\n'for n in sorted(names)))
for src,dest in [(lib,out/'libjb-json.a'),(lib,out/'libjb-json-unused.a')]:
 shutil.copy2(src,dest);subprocess.run(['llvm-objcopy-18','--redefine-syms='+str(mapping),str(dest)],check=True)
p=r/'jb-backend/src/catalog.cpp';s=p.read_text().replace('#include <json-c/json.h>','#include "jb_json_symbols.hpp"\n#include <json-c/json.h>');p.write_text(s)
p=r/'GNUmakefile';s=p.read_text().replace('sdl2 libcurl libarchive json-c','sdl2 libcurl libarchive').replace('APP_STATIC_ARCHIVES := .deps/ps5-opengl/libps5opengl-group.a','APP_STATIC_ARCHIVES := .deps/ps5-opengl/libps5opengl-group.a build/jb-json/libjb-json.a');p.write_text(s)
print('Isolated',len(names),'JSON-C symbols')
