from pathlib import Path
r=Path(__file__).resolve().parents[1]
import re,subprocess,os,tempfile
stage=r/'build/rar-tests';stage.mkdir(parents=True,exist_ok=True)
text=(r/'third_party/unrar/makefile').read_text();names=re.search(r'OBJECTS=(.*?)\n\n',text,re.S).group(1).replace('\\','').split()+['filestr.o','scantree.o','dll.o','qopen.o']
build=stage/'unrar-host';build.mkdir(exist_ok=True)
for name in names:
 source=r/'third_party/unrar'/name.replace('.o','.cpp');obj=build/name
 if not obj.exists():subprocess.run(['clang++-18','-std=c++11','-O1','-w','-fPIC','-D_UNIX','-DRARDLL','-c',str(source),'-o',str(obj)],check=True)
subprocess.run(['llvm-ar-18','rcs',str(build/'libunrar.a'),*[str(build/n) for n in names]],check=True)
code=r'''#include "../../jb-backend/src/rar_package.hpp"
#include <cassert>
#include "../../jb-backend/src/pkg.hpp"
namespace jbstore { const PkgBackend& pkg_backend(){static PkgBackend b;return b;} }
#include <filesystem>
#include <cstdio>
int main(int argc,char**argv){char msg[512]{};std::string dir=argv[1];for(const char* name:{"plain.rar","encrypted.rar","headers.rar"}){auto file=dir+"/"+name;bool ok=jbstore::rar_to_zip(file.c_str(),(dir+"/output.zip").c_str(),"demo-pass",msg,sizeof(msg),{});if(!ok)fprintf(stderr,"%s: %s\n",name,msg);assert(ok);assert(std::filesystem::exists(dir+"/output.zip"));}for(const char* name:{"encrypted.rar","headers.rar"}){assert(!jbstore::rar_to_zip((dir+"/"+name).c_str(),(dir+"/bad.zip").c_str(),"wrong",msg,sizeof(msg),{}));assert(!std::filesystem::exists(dir+"/bad.zip"));assert(!jbstore::rar_to_zip((dir+"/"+name).c_str(),(dir+"/bad.zip").c_str(),"",msg,sizeof(msg),{}));}assert(!jbstore::rar_to_zip((dir+"/plain.rar").c_str(),(dir+"/cancel.zip").c_str(),"",msg,sizeof(msg),{+[](const char*,std::uint64_t,std::uint64_t,void*){return false;},nullptr}));assert(!std::filesystem::exists(dir+"/cancel.zip"));assert(!jbstore::rar_to_zip((dir+"/links.rar").c_str(),(dir+"/links.zip").c_str(),"",msg,sizeof(msg),{}));assert(!std::filesystem::exists(dir+"/links.zip"));assert(!jbstore::rar_to_zip((dir+"/corrupt.rar").c_str(),(dir+"/corrupt.zip").c_str(),"",msg,sizeof(msg),{}));assert(!std::filesystem::exists(dir+"/corrupt.zip"));auto base=dir+"/installed";std::filesystem::create_directory(base);auto url="file://"+dir+"/encrypted.rar";if(!jbstore::install_url(url.c_str(),base.c_str(),"",msg,sizeof(msg),{},jbstore::PackageFormat::rar,"PPSA12345","Fixture",nullptr,0,false,"demo-pass")){fprintf(stderr,"Install: %s\n",msg);return 1;}assert(std::filesystem::exists(base+"/PPSA12345/eboot.bin"));assert(!jbstore::install_url(url.c_str(),base.c_str(),"",msg,sizeof(msg),{},jbstore::PackageFormat::rar,"PPSA12345","Fixture",nullptr,0,false,"wrong"));assert(std::filesystem::exists(base+"/PPSA12345/eboot.bin"));auto image="file://"+dir+"/container.rar";assert(jbstore::install_url(image.c_str(),base.c_str(),"",msg,sizeof(msg),{},jbstore::PackageFormat::rar,"PPSA54321","Fixture",nullptr,0,false,"demo-pass"));assert(std::filesystem::exists(base+"/PPSA54321.ffpfsc"));assert(std::filesystem::file_size(base+"/PPSA54321.ffpfsc")==26);assert(jbstore::package_format("https://example.invalid/test.RAR?token=x")==jbstore::PackageFormat::rar);puts("RAR5, encrypted data/headers, wrong/missing password and cancellation passed (AI-assisted)");}
'''
cpp=stage/'rar_test.cpp';cpp.write_text(code.replace("../../jb-backend/src/",str(r/"jb-backend/src")+"/"))
exe=build/'rar-test';subprocess.run(['clang++-18','-std=c++20',str(cpp),str(r/'src/app/rar_package.cpp'),str(r/'jb-backend/src/install.cpp'),str(r/'jb-backend/src/pkg.cpp'),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections',str(build/'libunrar.a'),'-larchive','-lcurl','-o',str(exe)],check=True)
rar=Path(os.environ['RAR_TEST_BIN']).resolve()
with tempfile.TemporaryDirectory(prefix='store-rar-',dir='/data') as directory:
 d=Path(directory);(d/'PPSA12345').mkdir();(d/'PPSA12345/eboot.bin').write_text('Owned fixture, not executable');(d/'PPSA12345/data.txt').write_text('Hello test '*100)
 for name,flags in [('plain.rar',[]),('encrypted.rar',['-pdemo-pass']),('headers.rar',['-hpdemo-pass'])]:subprocess.run([str(rar),'a','-idq',*flags,name,'PPSA12345'],cwd=d,check=True)
 (d/'link.txt').symlink_to('PPSA12345/data.txt')
 subprocess.run([str(rar),'a','-idq','-ol','links.rar','link.txt'],cwd=d,check=True)
 (d/'corrupt.rar').write_bytes((d/'plain.rar').read_bytes()[:32])
 (d/'image.ffpfsc').write_text('Owned fake container data!')
 subprocess.run([str(rar),'a','-idq','-pdemo-pass','container.rar','image.ffpfsc'],cwd=d,check=True)
 subprocess.run([str(exe),str(d)],check=True)
