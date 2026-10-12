// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) One-shot URI test based on itsPLK's unified autoloader.
// No target process calls, input injection, kill or executable patches.
#include <cstdint>
#include <cstdio>
#include <dlfcn.h>
#include <unistd.h>
#include <ps5/kernel.h>
extern "C" int sceKernelLoadStartModule(const char*,size_t,const void*,uint32_t,const void*,int*);
struct LaunchParam { uint32_t size; uint32_t userId; };
int main(){
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 void* user=dlopen("libSceUserService.sprx",RTLD_LAZY);
 auto init=user?reinterpret_cast<int(*)(const void*)>(dlsym(user,"sceUserServiceInitialize")):nullptr;
 auto fg=user?reinterpret_cast<int(*)(int*)>(dlsym(user,"sceUserServiceGetForegroundUser")):nullptr;
 if(!init||!fg)return 1;
 int priority=256,uid=-1;int rc=init(&priority);printf("HOME-URI user_init=%d\n",rc);
 rc=fg(&uid);printf("HOME-URI foreground_rc=%d user=%d\n",rc,uid);if(rc||uid<=0)return 1;
 int mod=sceKernelLoadStartModule("/system_ex/common_ex/lib/libSceShellUIUtil.sprx",0,nullptr,0,nullptr,nullptr);
 printf("HOME-URI module=%d\n",mod);if(mod<0)return 1;
 auto ui=reinterpret_cast<int(*)()>(kernel_dynlib_dlsym(-1,(uint32_t)mod,"sceShellUIUtilInitialize"));
 auto launch=reinterpret_cast<int(*)(const char*,LaunchParam*)>(kernel_dynlib_dlsym(-1,(uint32_t)mod,"sceShellUIUtilLaunchByUri"));
 if(!ui||!launch)return 1;
 printf("HOME-URI ui_init=%d\n",ui());LaunchParam param{sizeof(LaunchParam),(uint32_t)uid};
 puts("HOME-URI requesting navigateToHome; no kill or virtual buttons");fflush(stdout);
 rc=launch("pshomeui:navigateToHome?bootCondition=psButton",&param);
 printf("HOME-URI launch_rc=%d hex=%08x\n",rc,(unsigned)rc);return rc?1:0;
}
