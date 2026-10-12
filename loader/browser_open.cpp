// (AI-assisted) Open the PS5 browser in the verified SDK payload context.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include "../app/src/browser_launcher.hpp"
int main(int argc,char** argv){
 if(argc!=2||!argv[1]||strlen(argv[1])>=2048||(strncmp(argv[1],"https://",8)&&strncmp(argv[1],"http://",7))||strpbrk(argv[1],"\r\n"))return 1;
 void* u=dlopen("libSceUserService.sprx",RTLD_LAZY);void* s=dlopen("libSceSystemService.sprx",RTLD_LAZY);
 store::browser_launch::Api api{u?reinterpret_cast<int(*)(void*)>(dlsym(u,"sceUserServiceInitialize")):nullptr,s?reinterpret_cast<int(*)(const char*,void*)>(dlsym(s,"sceSystemServiceLaunchWebBrowser")):nullptr,u?reinterpret_cast<int(*)()>(dlsym(u,"sceUserServiceTerminate")):nullptr};
 char message[256]{};int rc=store::browser_launch::launch(api,argv[1],message,sizeof(message));
 printf("DEVBRIDGE/1 BROWSER RESULT %d %s\n",rc,message);return rc==0?0:1;
}
