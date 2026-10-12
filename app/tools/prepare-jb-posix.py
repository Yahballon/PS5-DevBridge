from pathlib import Path
import shutil,subprocess
r=Path(__file__).resolve().parents[1];out=r/'build/jb-json';lib=out/'libjb-posix.a';shutil.copy2(r/'.deps/native/ps5-payload-sdk/target/lib/libc.a',lib)
names=['dladdr','gmtime_r','mkstemps','mkstemp','openlog','closelog','getpwuid_r'];mapping=out/'posix-symbols.map';mapping.write_text(''.join(n+' jb_sdk_'+n+'\n'for n in names));subprocess.run(['llvm-objcopy-18','--redefine-syms='+str(mapping),str(lib)],check=True)
p=r/'tools/build.sh';s=p.read_text().replace('"$sdk_root/target/lib/libc.a" --end-group','"$root/build/jb-json/libjb-posix.a" --end-group');p.write_text(s)
