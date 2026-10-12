// (AI-assisted) Read-only navigation-message identity inspection on firmware 5.10.
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
static void inspect(pid_t pid,const char* name,uintptr_t fn){
 unsigned char code[5]{};if(!fn||kernel_proc_copyout(pid,fn,code,5)!=0||memcmp(code,"\xe9\xcb\x09\x01\x00",5))return;
 uint32_t from=0;int rc=kernel_proc_copyout(pid,fn+0x109d0+0x39ca4,&from,4);
 printf("NAV-CONTEXT pid=%d name=%s read_rc=%d source_app=%u hex=%08x\n",pid,name,rc,from,from);
 if(pid==getpid())for(uintptr_t off:{0x2adc2,0x2bd07}){char key[80]{};if(kernel_proc_copyout(pid,fn+0x109d0+off,key,sizeof(key))==0)printf("NAV-KEY %.79s\n",key);}
}
int main(){
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 void* lib=dlopen("libSceSystemService.sprx",RTLD_LAZY);if(!lib)return 1;inspect(getpid(),"payload",(uintptr_t)dlsym(lib,"sceSystemServiceNavigateToAnotherApp"));
 int mib[]={1,14,8,0};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)<0||!size||size>8*1024*1024)return 1;
 std::vector<unsigned char> bytes(size);if(sysctl(mib,4,bytes.data(),&size,nullptr,0)<0)return 1;
 for(size_t at=0;at+4<=size;){int len=0;memcpy(&len,bytes.data()+at,4);if(len<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||(size_t)len>size-at)return 1;
  kinfo_proc p{};memcpy(&p,bytes.data()+at,std::min(sizeof(p),(size_t)len));at+=len;p.ki_comm[sizeof(p.ki_comm)-1]=0;
  if(strcmp(p.ki_comm,"SceShellUI")&&strcmp(p.ki_comm,"eboot.bin"))continue;
  uint32_t handle=0;if(kernel_dynlib_handle(p.ki_pid,"libSceSystemService.sprx",&handle)==0)inspect(p.ki_pid,p.ki_comm,kernel_dynlib_dlsym(p.ki_pid,handle,"sceSystemServiceNavigateToAnotherApp"));
 }
 return 0;
}
