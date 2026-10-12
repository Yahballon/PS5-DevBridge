// (AI-assisted) Native DevBridge-context navigation: temporary call with full stack/register rollback.
// No ShellUI target, controller input, process termination or executable-code patch.
#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vector>
#include <algorithm>
#include <dlfcn.h>
#include <ps5/kernel.h>
static pid_t target_pid=-1;
static uint32_t expected_app_id=0;
static void note(const char* what,int value){printf("NATIVE-NAV %s: %d / %08x\n",what,value,(unsigned)value);fflush(stdout);}
#include "remote_call.hpp"
int main(){
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 void* lib=dlopen("libSceSystemService.sprx",RTLD_LAZY);void* us=dlopen("libSceUserService.sprx",RTLD_LAZY);if(!lib||!us)return 1;
 auto find=reinterpret_cast<int(*)(const char*)>(dlsym(lib,"sceLncUtilGetAppId"));auto title=reinterpret_cast<int(*)(int,char*)>(dlsym(lib,"sceLncUtilGetAppTitleId"));auto big=reinterpret_cast<int(*)()>(dlsym(lib,"sceLncUtilGetAppIdOfRunningBigApp"));
 auto init=reinterpret_cast<int(*)(void*)>(dlsym(us,"sceUserServiceInitialize"));auto user=reinterpret_cast<int(*)(int*)>(dlsym(us,"sceUserServiceGetForegroundUser"));auto term=reinterpret_cast<int(*)()>(dlsym(us,"sceUserServiceTerminate"));if(!find||!title||!big||!init||!user||!term)return 1;
 int id=find("PPSA99337");char name[256]{};if(id<=0||id!=big()||title(id,name)!=0||strcmp(name,"PPSA99337"))return 1;expected_app_id=id;
 int initialized=init(nullptr),uid=-1;if((initialized!=0&&(unsigned)initialized!=0x80960003u)||user(&uid)!=0||uid<0)return 1;
 int mib[]={1,14,8,0};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)<0||!size||size>8*1024*1024)return 1;
 std::vector<unsigned char> bytes(size);if(sysctl(mib,4,bytes.data(),&size,nullptr,0)<0)return 1;unsigned matches=0;
 for(size_t at=0;at+4<=size;){int len=0;memcpy(&len,bytes.data()+at,4);if(len<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||(size_t)len>size-at)return 1;
 kinfo_proc p{};memcpy(&p,bytes.data()+at,std::min(sizeof(p),(size_t)len));at+=len;p.ki_comm[sizeof(p.ki_comm)-1]=0;if(strcmp(p.ki_comm,"eboot.bin"))continue;
 uint32_t handle=0;if(kernel_dynlib_handle(p.ki_pid,"libSceSystemService.sprx",&handle)!=0)continue;uintptr_t fn=kernel_dynlib_dlsym(p.ki_pid,handle,"sceSystemServiceNavigateToAnotherApp");uint32_t cached=0;
 if(fn&&kernel_proc_copyout(p.ki_pid,fn+0x109d0+0x39ca4,&cached,4)==0&&cached==expected_app_id){target_pid=p.ki_pid;matches++;}
 }
 if(matches!=1||target_pid<=0){puts("NATIVE-NAV identity guard refused");if(initialized==0)term();return 1;}
 printf("NATIVE-NAV verified title=PPSA99337 pid=%d app_id=%u user_id=%d\n",target_pid,expected_app_id,uid);fflush(stdout);
 int rc=remote_navigate(uid);note("Result",rc);if(initialized==0)term();return rc==0?0:1;
}
