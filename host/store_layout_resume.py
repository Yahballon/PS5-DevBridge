from pathlib import Path
import json,re
r=Path(r'C:/Users/GETRICH/Desktop/JB-STORE-ProsperoUI')
def write(p,s):p.write_text(s,encoding='utf-8',newline='\n')
layout=r"""// (AI-assisted) Store-owned storage layout; migrate existing files without overwriting.
#pragma once
#include <string>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>
namespace store::storage {
inline bool directory(const std::string& p){struct stat s{};return mkdir(p.c_str(),0777)==0||(lstat(p.c_str(),&s)==0&&S_ISDIR(s.st_mode));}
inline void prepare(const std::string& root){
 if(!directory(root))return;for(const char* sub:{"logs","cache","tmp","covers"})directory(root+"/"+sub);
 DIR* dir=opendir(root.c_str());if(!dir)return;
 while(auto* e=readdir(dir)){std::string name=e->d_name;if(name=="."||name=="..")continue;std::string category;
 if(name=="log"){
  DIR* old=opendir((root+"/log").c_str());if(old){while(auto* f=readdir(old)){std::string n=f->d_name;if(n=="."||n=="..")continue;struct stat info{};std::string to=root+"/logs/"+n;if(lstat(to.c_str(),&info)!=0)std::rename((root+"/log/"+n).c_str(),to.c_str());}closedir(old);rmdir((root+"/log").c_str());}continue;
 }
 if(name.find(".log")!=name.npos)category="logs";
 else if(name.starts_with("cover-"))category="covers";
 else if(name.starts_with("github-")||name.starts_with("media-folder-")||name.starts_with("readme-")||name.starts_with("repo-")||name.starts_with("official-")||name.starts_with("single-json-")||name.starts_with("app-release-refresh-")||name=="store-update-manual-check.time")category="cache";
 else if(name.ends_with(".tmp")||name=="handled.txt")category="tmp";
 if(category.empty())continue;struct stat existing{};const auto target=root+"/"+category+"/"+name;
 if(lstat(target.c_str(),&existing)!=0)std::rename((root+"/"+name).c_str(),target.c_str());
 }closedir(dir);
}
}
"""
write(r/'src/core/storage_layout.hpp',layout)
p=r/'src/main.cpp';s=p.read_text();s='#include "core/storage_layout.hpp"\n'+s;s=s.replace('if (elevated && !store::diag::start(storage_root))','if(elevated)store::storage::prepare(storage_root);\n    if (elevated && !store::diag::start(storage_root))');s=s.replace('"/handled.txt"','"/tmp/handled.txt"');write(p,s)
# Move active log writers, including worker payloads, to the common log folder.
for directory in ['src','helper','examples/self-update-helper','jb-backend/src']:
 for p in (r/directory).rglob('*'):
  if p.suffix not in ('.cpp','.hpp','.c'):continue
  s=p.read_text();old=s
  s=s.replace('/data/jb-store/log/','/data/jb-store/logs/').replace('"/data/jb-store/log"','"/data/jb-store/logs"')
  s=re.sub(r'/data/jb-store/([A-Za-z0-9_-]+\.log)',r'/data/jb-store/logs/\1',s)
  if p.name=='jb_service.cpp':
   for name in ['artwork.log','self-update.log','shadowmount.log','app-updates.log']:s=s.replace('"/'+name+'"','"/logs/'+name+'"')
   for name in ['official-yahballon-zip','repo-','repo-manual-refresh.time']:s=s.replace('"/'+name+'"','"/cache/'+name+'"')
   s=s.replace('mkdir(cache.c_str(),0777);','mkdir(cache.c_str(),0777);mkdir((cache+"/cache").c_str(),0777);')
  if p.name=='catalog.cpp':
   for name in ['github-page-','github-page-pause-until','github-page-budget','single-json-cache.json','media-folder-v3-','readme-','app-release-refresh-','store-update-manual-check.time']:
    s=s.replace('"/'+name+'"','"/cache/'+name+'"')
   s=s.replace('"/cover-"','"/covers/cover-"')
  if p.name=='download_queue.cpp':s=s.replace('"%s/jb-store-download-%u.log", t.owner->base','"/data/jb-store/logs/jb-store-download-%u.log"');s=s.replace(' t.serial);',' t.serial);')
  if s!=old:write(p,s)
# All startup marker fallbacks are in a logs folder, never loose files.
p=r/'src/jb_startup.cpp';s=p.read_text();s=s.replace('extern "C" int sceKernelClose(int);','extern "C" int sceKernelClose(int);\nextern "C" int sceKernelMkdir(const char*,int);')
a=s.index('for(const char* path:');b=s.index('{boot_fd=sceKernelOpen',a)
s=s[:a]+'for(const char* directory:{"/data/jb-store/logs","/download0/logs","/app0/logs"}){sceKernelMkdir(directory,0777);char path[256];std::snprintf(path,sizeof(path),"%s/startup.log",directory);boot_fd=sceKernelOpen(path,O_WRONLY|O_CREAT|O_TRUNC,0777);if(boot_fd>=0)break;}}if(boot_fd>=0){sceKernelWrite(boot_fd,stage,std::strlen(stage));sceKernelWrite(boot_fd,"\\n",1);}}\n__attribute__((constructor(101))) static void early_boot(){jb_startup_mark("JB STORE (AI-assisted): constructors entered");}\n'
# Construct explicit function instead of retaining the old suffix.
s=s[:s.index('extern "C" void jb_startup_mark')]+s[s.index('extern "C" void jb_startup_mark'):]
write(p,s)
# Resume interrupted live package transfers; validators prevent joining different entities.
p=r/'jb-backend/src/install.cpp';s=p.read_text();s=s.replace('    bool failed = false;','    bool failed = false;\n    std::uint64_t offset = 0;')
s=s.replace('    t.current = now > 0 ? now : 0;\n    t.total = total > 0 ? total : 0;','    t.current = t.offset + (now > 0 ? now : 0);\n    t.total = total > 0 ? t.offset + total : 0;')
s=s.replace('            auto elapsed = now - last;','            auto elapsed = now - last;\n            // Console suspension is not active download time.\n            if(elapsed>5000){elapsed=0;window_ms=0;window_bytes=t.written;}')
old='    auto rc = progress.paused ? controlled_transfer(c, t) : curl_easy_perform(c);'
new=r"""    CURLcode rc=CURLE_FAILED_INIT;
    // (AI-assisted) Reconnect after app suspension/network loss using the same
    // staging file. libcurl refuses ignored ranges instead of appending a 200 body.
    for(unsigned attempt=0;attempt<31;++attempt){
        t.offset=t.written;t.current=t.offset;t.failed=false;
        curl_easy_setopt(c,CURLOPT_RESUME_FROM_LARGE,static_cast<curl_off_t>(t.offset));
        error[0]=0;
        rc=progress.paused?controlled_transfer(c,t):curl_easy_perform(c);
        if(rc==CURLE_OK)break;
        if(!progress.paused||t.failed||rc==CURLE_ABORTED_BY_CALLBACK)break;
        if(rc==CURLE_RANGE_ERROR&&t.offset){
            progress.note("Server refused resume; restarting safely");
            if(fflush(f)||ftruncate(fileno(f),0)||fseeko(f,0,SEEK_SET)){rc=CURLE_WRITE_ERROR;break;}
            clearerr(f);t.written=t.offset=0;
        }else if(rc!=CURLE_RECV_ERROR&&rc!=CURLE_SEND_ERROR&&rc!=CURLE_PARTIAL_FILE&&rc!=CURLE_OPERATION_TIMEDOUT&&rc!=CURLE_COULDNT_CONNECT&&rc!=CURLE_GOT_NOTHING)break;
        if(attempt==30)break;
        progress.note("Connection interrupted; continuing from saved bytes");
        for(unsigned wait=0;wait<20;++wait){if(!progress.report("Reconnecting",t.written,t.total)){rc=CURLE_ABORTED_BY_CALLBACK;break;}usleep(100000);}
        if(rc==CURLE_ABORTED_BY_CALLBACK)break;
        if(fflush(f)){rc=CURLE_WRITE_ERROR;break;}
    }
"""
assert old in s;s=s.replace(old,new)
# Large downloads stay on the selected install drive, inside the Store's tmp
# folder there, to preserve atomic publication and avoid filling internal /data.
old='    if (std::snprintf(job, sizeof(job), "%s/.hba-download-XXXXXX", base) >= int(sizeof(job)))'
new='    char temporary_root[768];std::snprintf(temporary_root,sizeof(temporary_root),"%s/.jb-store/tmp",base);\n    char create[768];std::snprintf(create,sizeof(create),"%s/job",temporary_root);\n    if(!parents(create)){std::snprintf(message,capacity,"Cannot create Store temporary folder");return false;}\n    if (std::snprintf(job, sizeof(job), "%s/download-XXXXXX", temporary_root) >= int(sizeof(job)))'
assert old in s;s=s.replace(old,new);write(p,s)
p=r/'sce_sys/param.json';j=json.loads(p.read_text());j['contentVersion']='01.000.098';write(p,json.dumps(j,indent=2)+'\n')
print('Applied storage migration and suspended-transfer reconnect changes')
