// (AI-assisted) One-shot, title-guarded browser lifecycle close; no pad or process-kill syscalls.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <ps5/kernel.h>
int main(){
 if(kernel_get_fw_version()!=0x05100023u){puts("BROWSER-API unsupported firmware; no close attempted");return 1;}
 void* lib=dlopen("libSceSystemService.sprx",RTLD_LAZY);if(!lib)return 1;
 auto find=reinterpret_cast<int(*)(const char*)>(dlsym(lib,"sceLncUtilGetAppId"));
 auto title=reinterpret_cast<int(*)(int,char*)>(dlsym(lib,"sceLncUtilGetAppTitleId"));
 auto big=reinterpret_cast<int(*)()>(dlsym(lib,"sceLncUtilGetAppIdOfRunningBigApp"));
 auto close=reinterpret_cast<int(*)(int)>(dlsym(lib,"sceLncUtilKillApp"));
 if(!find||!title||!big||!close)return 1;
 int browser=find("NPXS40087"),foreground=big();char identity[256]{};
 if(browser<=0||browser==foreground||title(browser,identity)!=0||strcmp(identity,"NPXS40087")){puts("BROWSER-API identity guard refused; no close attempted");return 1;}
 printf("BROWSER-API target=%s app_id=%d preserved_big_app=%d\n",identity,browser,foreground);fflush(stdout);
 // Hardware evidence: NPXS40087 owns ShellUI; this API restarts the shell.
 puts("BROWSER-API refused: NPXS40087 lifecycle close restarts ShellUI; no close attempted");
 return 2;
 int rc=close(browser);printf("BROWSER-API close_rc=%d hex=%08x\n",rc,(unsigned)rc);
 return rc==0?0:1;
}
