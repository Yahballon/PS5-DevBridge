from pathlib import Path
import json
root=Path(r'C:/Users/GETRICH/Desktop/JB-STORE-ProsperoUI')
main=r"""// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) URI browser close and one SystemService return request.
// Based on itsPLK unified-autoloader and phantomptr/ps5upload launch context.
#include "protocol.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ps5/kernel.h>
struct StoreAppInfo{std::uint32_t app_id;std::uint64_t unknown;char title_id[14];char rest[0x3c];};
extern "C" int sceKernelGetAppInfo(pid_t,StoreAppInfo*);
extern "C" int sceKernelLoadStartModule(const char*,size_t,const void*,uint32_t,const void*,int*);
struct UriParam{uint32_t size,user;};
struct AppParam{uint32_t size,user,options;uint64_t crash;uint32_t flags;};
static_assert(sizeof(AppParam)==32);
static void note(const char* text,int rc){
 printf("[browser-close] %s: %d / %08x\n",text,rc,(unsigned)rc);fflush(stdout);
 mkdir("/data/jb-store",0755);mkdir("/data/jb-store/log",0755);
 if(FILE* f=fopen("/data/jb-store/log/browser-close.log","a")){fprintf(f,"[browser-close] %s: %d / %08x\n",text,rc,(unsigned)rc);fclose(f);}
}
static bool owned(uint32_t pid,uint32_t expected=0){StoreAppInfo info{};return pid&&pid!=(uint32_t)getpid()&&sceKernelGetAppInfo(pid,&info)==0&&!memcmp(info.title_id,"FAKE00666",9)&&info.title_id[9]==0&&(!expected||info.app_id==expected);}
static int close_return(uint32_t pid){
 StoreAppInfo info{};if(!owned(pid)||sceKernelGetAppInfo(pid,&info))return -1;
 const uint32_t app_id=info.app_id;
 void* user=dlopen("libSceUserService.sprx",RTLD_LAZY);void* system=dlopen("libSceSystemService.sprx",RTLD_LAZY);
 auto init=user?reinterpret_cast<int(*)(const void*)>(dlsym(user,"sceUserServiceInitialize")):nullptr;
 auto fg=user?reinterpret_cast<int(*)(int*)>(dlsym(user,"sceUserServiceGetForegroundUser")):nullptr;
 auto terminate=user?reinterpret_cast<int(*)()>(dlsym(user,"sceUserServiceTerminate")):nullptr;
 auto launch=system?reinterpret_cast<int(*)(const char*,const char**,AppParam*)>(dlsym(system,"sceSystemServiceLaunchApp")):nullptr;
 if(!init||!fg||!terminate||!launch)return -1;
 int priority=256;int initialized=init(&priority);note("UserService initialize",initialized);
 if(initialized!=0&&(unsigned)initialized!=0x80960003u)return -1;
 int result=-1;int uid=-1;
 do{
  if(fg(&uid)||uid<=0)break;
  int mod=sceKernelLoadStartModule("/system_ex/common_ex/lib/libSceShellUIUtil.sprx",0,nullptr,0,nullptr,nullptr);
  note("ShellUIUtil load",mod);if(mod<0)break;
  auto ui=reinterpret_cast<int(*)()>(kernel_dynlib_dlsym(-1,(uint32_t)mod,"sceShellUIUtilInitialize"));
  auto uri=reinterpret_cast<int(*)(const char*,UriParam*)>(kernel_dynlib_dlsym(-1,(uint32_t)mod,"sceShellUIUtilLaunchByUri"));
  if(!ui||!uri)break;
  int rc=ui();note("ShellUIUtil initialize",rc);if(rc!=0)break;
  if(!owned(pid,app_id))break;
  UriParam home{sizeof(UriParam),(uint32_t)uid};
  rc=uri("pshomeui:navigateToHome?bootCondition=psButton",&home);note("Browser home navigation",rc);if(rc!=0)break;
  // Matches the twice-verified console sequence; needs testing on other firmware.
  usleep(1000000);
  if(!owned(pid,app_id))break;
  AppParam app{sizeof(AppParam),(uint32_t)uid,0,0,0};const char* args[]={nullptr};
  rc=launch("FAKE00666",args,&app);note("Return to running Store",rc);
  // Already-running is expected when foregrounding. Never retry: a second
  // launch request can immediately background the app again.
  result=(rc>=0||(unsigned)rc==0x8094000cu)?0:-3;
 }while(false);
 if(initialized==0)terminate();return result;
}
int main(){
 store::browser_close_wire::Request request;auto* bytes=reinterpret_cast<unsigned char*>(&request);size_t at=0;
 while(at<sizeof(request)){ssize_t n=read(STDIN_FILENO,bytes+at,sizeof(request)-at);if(n<=0)return 1;at+=n;}
 int rc=-1;
 if(request.magic==0x4a42434c&&request.version==3&&request.flags<=2&&request.nonce&&owned(request.pid)){
  // Legacy format flags remain wire-compatible; URI closing needs no X/O input.
  rc=kernel_get_fw_version()==0x05100023u?close_return(request.pid):-2;
 }
 printf("JBCLOSE/1 %llu %d\n",(unsigned long long)request.nonce,rc);fflush(stdout);return rc==0?0:1;
}
"""
(root/'helper/browser-close/main.cpp').write_text(main,encoding='utf-8',newline='\n')
p=root/'helper/browser-close/Makefile';p.write_text(p.read_text().replace('main.cpp one_shot.hpp remote_bind.hpp protocol.hpp','main.cpp protocol.hpp'),encoding='utf-8',newline='\n')
p=root/'src/platform/ps5/browser_close.cpp';s=p.read_text().replace('"Browser close completed"','"Browser closed; returning to Store"').replace('status==-2?','status==-3?"Browser closed, but returning to Store failed. Open the Store from Home; the download continues.":status==-2?');p.write_text(s,encoding='utf-8',newline='\n')
p=root/'sce_sys/param.json';data=json.loads(p.read_text());data['contentVersion']='01.000.097';p.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8',newline='\n')
print('Store URI close/return integration applied; package version 01.000.097')
