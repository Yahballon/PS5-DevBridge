// (AI-assisted) Normal launch request for verified running DevBridge only.
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ps5/kernel.h>
struct Param{uint32_t size;int user;uint32_t options;uint64_t crash;uint32_t flags;};
static_assert(sizeof(Param)==32);
int main(){
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 void* lib=dlopen("libSceSystemService.sprx",RTLD_LAZY);void* us=dlopen("libSceUserService.sprx",RTLD_LAZY);
 auto find=lib?reinterpret_cast<int(*)(const char*)>(dlsym(lib,"sceLncUtilGetAppId")):nullptr;
 auto title=lib?reinterpret_cast<int(*)(int,char*)>(dlsym(lib,"sceLncUtilGetAppTitleId")):nullptr;
 auto launch=lib?reinterpret_cast<int(*)(const char*,const char**,Param*)>(dlsym(lib,"sceSystemServiceLaunchApp")):nullptr;
 auto init=us?reinterpret_cast<int(*)(void*)>(dlsym(us,"sceUserServiceInitialize")):nullptr;
 auto fg=us?reinterpret_cast<int(*)(int*)>(dlsym(us,"sceUserServiceGetForegroundUser")):nullptr;
 if(!find||!title||!launch||!init||!fg)return 1;
 int id=find("PPSA99337");char name[256]{};if(id<=0||title(id,name)||strcmp(name,"PPSA99337"))return 1;
 int priority=256;init(&priority);int uid=-1;if(fg(&uid)||uid<=0)return 1;
 Param p{sizeof(Param),uid,0,0,0};printf("APP-LAUNCH verified running app=%s id=%d api=sceSystemServiceLaunchApp flags=0\n",name,id);fflush(stdout);
 const char* args[]={nullptr};int rc=launch("PPSA99337",args,&p);printf("APP-LAUNCH result=%d hex=%08x\n",rc,(unsigned)rc);return rc<0?1:0;
}
