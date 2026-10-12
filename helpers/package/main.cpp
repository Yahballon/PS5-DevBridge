// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) System package operations, ABI per etaHEN pkg-writeup.md.
#include <ps5/kernel.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <sys/stat.h>
extern "C" int sceKernelLoadStartModule(const char*,size_t,const void*,unsigned,void*,int*);
struct Meta{const char* uri;const char* ex;const char* scenario;const char* id;const char* name;const char* icon;};
struct Pkg{char id[48];int type,platform;};
struct PlayGo{char langs[30][8],scenarios[64][3],ids[64][48];unsigned char unknown[6480];};
struct Error{int32_t code,version;char description[512],type[9];};
struct Status{char status[16],src[8];uint32_t remain;uint64_t downloaded,initial,total;uint32_t promote;Error error;int32_t copy;bool copy_only;};
int main(){struct {uint32_t op;char value[1024];} r{};size_t at=0;while(at<sizeof(r)){auto n=read(0,(char*)&r+at,sizeof(r)-at);if(n<=0)return 1;at+=n;}
 if(!memchr(r.value,0,sizeof(r.value)))return 1;
 if(r.op==15){if(strlen(r.value)!=9||!strncmp(r.value,"NPXS",4))return 1;for(int i=0;i<9;i++)if(i<4?(r.value[i]<'A'||r.value[i]>'Z'):(r.value[i]<'0'||r.value[i]>'9'))return 1;}
 else if(r.op==14){struct stat st{};size_t n=strlen(r.value);if(strstr(r.value,"..")||n<4||strcmp(r.value+n-4,".pkg")||(strncmp(r.value,"/data/",6)&&strncmp(r.value,"/mnt/usb",8))||lstat(r.value,&st)||!S_ISREG(st.st_mode))return 1;}
 else if(r.op==19){if(strlen(r.value)!=36)return 1;}else return 1;
 int mod=sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx",0,nullptr,0,nullptr,nullptr);
 if(mod<0){printf("PACKAGE ERROR load %08x\n",mod);return 1;}
 auto init=(int(*)())kernel_dynlib_dlsym(getpid(),mod,"sceAppInstUtilInitialize");int rc=init?init():-1;
 if(rc){printf("PACKAGE ERROR initialize %08x\n",rc);return 1;}
 if(r.op==15){auto fn=(int(*)(const char*))kernel_dynlib_dlsym(getpid(),mod,"sceAppInstUtilAppUnInstall");rc=fn?fn(r.value):-1;printf("PACKAGE UNINSTALL result=%08x\n",rc);}
 else if(r.op==14){auto fn=(int(*)(Meta*,Pkg*,PlayGo*))kernel_dynlib_dlsym(getpid(),mod,"sceAppInstUtilInstallByPackage");Meta meta{r.value,"","","",strrchr(r.value,'/')+1,""};Pkg pkg{};PlayGo play{};rc=fn?fn(&meta,&pkg,&play):-1;printf("PACKAGE INSTALL accepted=%d result=%08x content_id=%.48s\n",rc==0,rc,pkg.id);}
 else{auto fn=(int(*)(const char*,Status*))kernel_dynlib_dlsym(getpid(),mod,"sceAppInstUtilGetInstallStatus");Status s{};rc=fn?fn(r.value,&s):-1;printf("PACKAGE STATUS result=%08x state=%.16s downloaded=%llu total=%llu error=%08x\n",rc,s.status,(unsigned long long)s.downloaded,(unsigned long long)s.total,s.error.code);}
 return rc?1:0;}
