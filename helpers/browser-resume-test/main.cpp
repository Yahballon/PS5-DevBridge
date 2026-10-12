// (AI-assisted) Single native-app-only resume request; no kill, pad or ptrace calls.
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <ps5/kernel.h>
int main(){
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 void* lib=dlopen("libSceSystemService.sprx",RTLD_LAZY);if(!lib)return 1;
 auto find=reinterpret_cast<int(*)(const char*)>(dlsym(lib,"sceLncUtilGetAppId"));
 auto title=reinterpret_cast<int(*)(int,char*)>(dlsym(lib,"sceLncUtilGetAppTitleId"));
 auto big=reinterpret_cast<int(*)()>(dlsym(lib,"sceLncUtilGetAppIdOfRunningBigApp"));
 auto focus=reinterpret_cast<int(*)(int,std::uintptr_t)>(dlsym(lib,"sceLncUtilResumeApp"));
 if(!find||!title||!big||!focus)return 1;
 int id=find("PPSA99337");char name[256]{};
 if(id<=0||id!=big()||title(id,name)!=0||strcmp(name,"PPSA99337")){puts("RESUME-TEST refused: native DevBridge is not the running app");return 1;}
 printf("RESUME-TEST target=%s id=%d optional_argument=0\n",name,id);fflush(stdout);
 int result=focus(id,0);printf("RESUME-TEST result=%d hex=%08x\n",result,(unsigned)result);return result==0?0:1;
}
