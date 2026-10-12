// (AI-assisted) Read-only foreground app identification.
#include <dlfcn.h>
#include <cstdio>
int main(){void* lib=dlopen("libSceSystemService.sprx",RTLD_LAZY);auto big=lib?(int(*)())dlsym(lib,"sceLncUtilGetAppIdOfRunningBigApp"):nullptr;auto title=lib?(int(*)(int,char*))dlsym(lib,"sceLncUtilGetAppTitleId"):nullptr;if(!big||!title)return 1;int id=big();char name[256]{};int rc=title(id,name);printf("BIG APP id=%d title=%s result=%08x\n",id,name,rc);return 0;}
