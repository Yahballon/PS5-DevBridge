// (AI-assisted) Read-only browser process comparison; never kills or patches a process.
#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <dlfcn.h>
#include <ps5/kernel.h>
int main(){
 int mib[]={1,14,8,0};size_t size=0;
 if(sysctl(mib,4,nullptr,&size,nullptr,0)<0||!size||size>8*1024*1024)return 1;
 std::vector<unsigned char> data(size);if(sysctl(mib,4,data.data(),&size,nullptr,0)<0)return 1;
 printf("PROCESS-SNAPSHOT/1 self=%d\n",getpid());
 for(size_t at=0;at+4<=size;){int length=0;memcpy(&length,data.data()+at,4);
  if(length<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||(size_t)length>size-at)return 1;
  kinfo_proc p{};memcpy(&p,data.data()+at,std::min(sizeof(p),(size_t)length));at+=length;p.ki_comm[sizeof(p.ki_comm)-1]=0;
  printf("PROC pid=%d ppid=%d name=%s\n",p.ki_pid,p.ki_ppid,p.ki_comm);
 }
 for(const char* library:{"libSceSystemService.sprx","libSceWebBrowserDialog.sprx","libSceLncUtil.sprx"}){
  void* lib=dlopen(library,RTLD_LAZY);printf("LIB %s loaded=%d\n",library,lib!=nullptr);
  if(!lib)continue;
  for(const char* name:{"sceSystemServiceCloseWebBrowser","sceSystemServiceHideWebBrowser","sceSystemServiceFinishWebBrowser","sceWebBrowserDialogClose","sceWebBrowserDialogTerminate","sceLncUtilKillApp","sceLncUtilTerminateApp"})printf("EXPORT %s %s present=%d\n",library,name,dlsym(lib,name)!=nullptr);
 }
 void* sys=dlopen("libSceSystemService.sprx",RTLD_LAZY);void* web=dlopen("libSceWebBrowserDialog.sprx",RTLD_LAZY);
 for(const char* name:{"sceSystemServiceLaunchWebBrowser","sceSystemServiceLaunchWebApp","sceLncUtilGetAppId","sceLncUtilGetAppTitleId","sceLncUtilGetAppStatus","sceLncUtilKillApp","sceLncUtilSetAppFocus","sceLncUtilResumeApp","sceSystemServiceNavigateToAnotherApp","sceSystemModalDialogClose","sceSystemModalDialogOpen","sceWebBrowserDialogGetStatus","sceWebBrowserDialogClose"}){
  void* fn=dlsym(strncmp(name,"sceWeb",6)?sys:web,name);unsigned char code[2048]{};
  if(fn&&kernel_proc_copyout(getpid(),(uintptr_t)fn,code,sizeof(code))==0){printf("CODE %s %llx ",name,(unsigned long long)(uintptr_t)fn);for(unsigned char b:code)printf("%02x",b);printf("\n");}
 }
 auto status=web?reinterpret_cast<int(*)()>(dlsym(web,"sceWebBrowserDialogGetStatus")):nullptr;
 if(status)printf("BROWSER-DIALOG-STATUS %d\n",status());
for(size_t at=0;at+4<=size;){int length=0;memcpy(&length,data.data()+at,4);if(length<=0||(size_t)length>size-at)break;kinfo_proc p{};memcpy(&p,data.data()+at,std::min(sizeof(p),(size_t)length));at+=length;
 if(strcmp(p.ki_comm,"SceShellUI")&&strcmp(p.ki_comm,"SceShellCore")&&strcmp(p.ki_comm,"SceNKUIProcess"))continue;
 uint32_t handle=0;int rc=kernel_dynlib_handle(p.ki_pid,"libSceWebBrowserDialog.sprx",&handle);printf("TARGET-WEBLIB pid=%d name=%s rc=%d handle=%u\n",p.ki_pid,p.ki_comm,rc,handle);
}
void* get=dlsym(sys,"sceLncUtilGetAppId");if(get){unsigned char code[1024]{};if(kernel_proc_copyout(getpid(),(uintptr_t)get+0x2e90,code,sizeof(code))==0){printf("INNER-GETAPPID ");for(unsigned char b:code)printf("%02x",b);printf("\n");}}
auto appid=sys?reinterpret_cast<int(*)(const char*)>(dlsym(sys,"sceLncUtilGetAppId")):nullptr;
 auto title=sys?reinterpret_cast<int(*)(int,char*)>(dlsym(sys,"sceLncUtilGetAppTitleId")):nullptr;
 if(appid)for(const char* name:{"NPXS40087","PPSA99337"}){int id=appid(name);printf("TITLE-LOOKUP %s id=%d hex=%08x\n",name,id,(unsigned)id);if(id>0 && title){char verified[256]={0};int rc=title(id,verified);printf("TITLE-VERIFY id=%d rc=%d title=%.255s\n",id,rc,verified);}}
for(const char* name:{"sceSystemServiceLaunchWebBrowser","sceSystemModalDialogOpen","sceSystemModalDialogClose"}){
 uintptr_t fn=(uintptr_t)dlsym(sys,name);unsigned char code[1024]{};if(!fn||kernel_proc_copyout(getpid(),fn,code,sizeof(code))!=0)continue;
 for(unsigned i=0;i+7<sizeof(code);i++){
  if((code[i]==0x48||code[i]==0x4c)&&code[i+1]==0x8d&&(code[i+2]&0xc7)==0x05){int32_t off=0;memcpy(&off,code+i+3,4);char str[96]{};
   if(kernel_proc_copyout(getpid(),fn+i+7+off,str,sizeof(str))!=0)continue;str[95]=0;
   size_t len=strnlen(str,96);if(len>=3&&len<95){bool printable=true;for(size_t j=0;j<len;j++)if((unsigned char)str[j]<32||(unsigned char)str[j]>126)printable=false;if(printable)printf("REF %s +%x %s\n",name,i,str);}
  }
 }
}
void* focus=dlsym(sys,"sceLncUtilSetAppFocus");if(focus){unsigned char code[1024]{};if(kernel_proc_copyout(getpid(),(uintptr_t)focus+0x3640,code,sizeof(code))==0){printf("INNER-FOCUS ");for(unsigned char b:code)printf("%02x",b);printf("\n");}}
void* browser=dlsym(sys,"sceSystemServiceLaunchWebBrowser");if(browser){unsigned char code[2048]{};if(kernel_proc_copyout(getpid(),(uintptr_t)browser+0xfa90,code,sizeof(code))==0){printf("INNER-WEBAPP ");for(unsigned char b:code)printf("%02x",b);printf("\n");}}
void* f=dlsym(sys,"sceLncUtilSetAppFocus");if(f){unsigned char code[1024]{};if(kernel_proc_copyout(getpid(),(uintptr_t)f+0x3640+0x4800,code,sizeof(code))==0){printf("FOCUS-MARSHAL ");for(unsigned char b:code)printf("%02x",b);printf("\n");}}
void* nav=dlsym(sys,"sceSystemServiceNavigateToAnotherApp");if(nav){unsigned char code[2048]{};if(kernel_proc_copyout(getpid(),(uintptr_t)nav+0x109d0,code,sizeof(code))==0){printf("INNER-NAVIGATE ");for(unsigned char b:code)printf("%02x",b);printf("\n");}}
 printf("PROCESS-SNAPSHOT/1 DONE\n");return 0;
}
