// (AI-assisted) Bounded read-only klog observation around one browser launch.
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include <dlfcn.h>
#include "../../app/src/browser_launcher.hpp"
extern "C" {
#include "../../loader/notify.h"
}
int main(){
 int fd=open("/dev/klog",O_RDONLY|O_NONBLOCK);if(fd<0)return 1;
 char block[4096];for(unsigned i=0;i<64;i++)if(read(fd,block,sizeof(block))<=0)break;
 void* u=dlopen("libSceUserService.sprx",RTLD_LAZY);void* s=dlopen("libSceSystemService.sprx",RTLD_LAZY);
 store::browser_launch::Api api{u?reinterpret_cast<int(*)(void*)>(dlsym(u,"sceUserServiceInitialize")):nullptr,s?reinterpret_cast<int(*)(const char*,void*)>(dlsym(s,"sceSystemServiceLaunchWebBrowser")):nullptr,u?reinterpret_cast<int(*)()>(dlsym(u,"sceUserServiceTerminate")):nullptr};
 char message[256]{};int rc=store::browser_launch::launch(api,"https://example.com/?ps5-close-probe=1",message,sizeof(message));printf("BROWSER-REFERENCE launch_rc=%d %s\n",rc,message);
 setvbuf(stdout,nullptr,_IONBF,0);
 notify("PS5 DevBridge: trace active. Close the test browser with Circle now.");
 puts("REFERENCE TRACE ACTIVE - manual close only; no injected input");
 size_t total=0;for(unsigned i=0;i<3000&&total<256*1024;i++){
  ssize_t n=read(fd,block,sizeof(block));if(n>0){fwrite(block,1,n,stdout);total+=n;}else if(n<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)break;
  usleep(10000);
 }
 close(fd);printf("\nBROWSER-REFERENCE complete bytes=%zu\n",total);return 0;
}
