// (AI-assisted) One bounded, firmware-byte-guarded SDK web-view close experiment.
// No controller, ptrace, kill-app or process-signalling calls.
#include <ps5/kernel.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "signature.hpp"
#include "../../app/src/browser_launcher.hpp"
int main(){
 if(kernel_get_fw_version()!=0x05100023u){puts("PRIVATE-CLOSE unsupported firmware");return 1;}
 void* u=dlopen("libSceUserService.sprx",RTLD_LAZY);void* s=dlopen("libSceSystemService.sprx",RTLD_LAZY);
 if(!u||!s)return 1;
 auto browser=dlsym(s,"sceSystemServiceLaunchWebBrowser");if(!browser)return 1;
 uintptr_t close_address=(uintptr_t)browser+0xfa90+0xb0;
 unsigned char actual[sizeof(expected_close)]{};
 if(kernel_proc_copyout(getpid(),close_address,actual,sizeof(actual))!=0||memcmp(actual,expected_close,sizeof(actual))){puts("PRIVATE-CLOSE signature mismatch; no close attempted");return 1;}
 store::browser_launch::Api api{reinterpret_cast<int(*)(void*)>(dlsym(u,"sceUserServiceInitialize")),reinterpret_cast<int(*)(const char*,void*)>(browser),reinterpret_cast<int(*)()>(dlsym(u,"sceUserServiceTerminate"))};
 char message[256]{};int opened=store::browser_launch::launch(api,"https://example.com/",message,sizeof(message));
 printf("PRIVATE-CLOSE own test page opened rc=%d %s\n",opened,message);fflush(stdout);if(opened!=0)return 1;
 sleep(3);
 puts("PRIVATE-CLOSE invoking verified no-argument web-view close routine");fflush(stdout);
 auto close=reinterpret_cast<int(*)()>(close_address);int result=close();
 printf("PRIVATE-CLOSE result=%d hex=%08x\n",result,(unsigned)result);return result==0?0:1;
}
