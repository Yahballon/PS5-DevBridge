// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) Only explicitly registered homebrew folders may be managed.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <cstdlib>
struct Request{uint32_t action;char title[16],path[1024];};
struct Param{uint32_t size;int user;uint32_t options;uint64_t crash;uint32_t flags;};
static bool title(const char* t){if(strlen(t)!=9||!strncmp(t,"NPXS",4))return false;for(int i=0;i<9;i++)if(i<4?(t[i]<'A'||t[i]>'Z'):(t[i]<'0'||t[i]>'9'))return false;return true;}
static bool metadata(const std::string& path,const char* id){struct stat st{};if(lstat(path.c_str(),&st)||!S_ISDIR(st.st_mode))return false;FILE* f=fopen((path+"/sce_sys/param.json").c_str(),"r");if(!f)return false;char b[8192]{};fread(b,1,sizeof(b)-1,f);fclose(f);char* at=strstr(b,"\"titleId\"");if(!at)return false;at=strchr(at,':');if(!at)return false;at=strchr(at,'"');return at&&!strncmp(at+1,id,9)&&at[10]=='"';}
static bool root_path(const char* p,const char* id){char want[1024];snprintf(want,sizeof(want),"/data/homebrew/%s",id);if(!strcmp(p,want))return true;for(int i=0;i<8;i++){snprintf(want,sizeof(want),"/mnt/usb%d/homebrew/%s",i,id);if(!strcmp(p,want))return true;}return false;}
int main(){setvbuf(stdout,nullptr,_IONBF,0);Request q{};size_t n=0;while(n<sizeof(q)){auto got=read(0,(char*)&q+n,sizeof(q)-n);if(got<=0)return 1;n+=got;}if(!memchr(q.title,0,16)||!memchr(q.path,0,1024)||q.action>8)return 1;
 mkdir("/data/ps5-devbridge",0755);mkdir("/data/ps5-devbridge/managed",0755);
 if(!q.action){DIR* d=opendir("/data/ps5-devbridge/managed");if(d){while(auto* e=readdir(d)){if(!title(e->d_name))continue;FILE* f=fopen((std::string("/data/ps5-devbridge/managed/")+e->d_name).c_str(),"r");char path[1024]{};if(f){fgets(path,sizeof(path),f);fclose(f);printf("APP %s path=%s\n",e->d_name,path);}}closedir(d);}return 0;}
 if(!title(q.title))return 1;std::string marker=std::string("/data/ps5-devbridge/managed/")+q.title;char path[1024]{};
 if(q.action==1){if(!root_path(q.path,q.title)||!metadata(q.path,q.title))return 1;FILE* f=fopen(marker.c_str(),"w");if(!f)return 1;fputs(q.path,f);fclose(f);puts("APP REGISTERED");return 0;}
 if(q.action==7){if(!root_path(q.path,q.title))return 1;std::string stage=std::string(q.path)+".devbridge-stage";if(mkdir(stage.c_str(),0755)){puts("APP ERROR staging exists or parent missing");return 1;}FILE* marker_file=fopen(marker.c_str(),"w");if(!marker_file)return 1;fputs(q.path,marker_file);fclose(marker_file);puts("APP STAGE READY");return 0;}
 FILE* f=fopen(marker.c_str(),"r");if(!f)return 1;fgets(path,sizeof(path),f);fclose(f);if(!root_path(path,q.title))return 1;
 if(q.action==8){std::string stage=std::string(path)+".devbridge-stage/";if(strncmp(q.path,stage.c_str(),stage.size())||strstr(q.path,".."))return 1;char parent[1024],resolved[1024];snprintf(parent,sizeof(parent),"%s",q.path);char* slash=strrchr(parent,'/');if(!slash)return 1;*slash=0;if(!realpath(parent,resolved)||(std::string(resolved)+"/").rfind(stage,0)!=0)return 1;int rc=mkdir(q.path,0755);printf("APP MKDIR result=%d\n",rc);return rc?1:0;}
 if(q.action!=4&&!metadata(path,q.title))return 1;
 void* lib=dlopen("libSceSystemService.sprx",RTLD_LAZY);auto find=lib?(int(*)(const char*))dlsym(lib,"sceLncUtilGetAppId"):nullptr;auto gettitle=lib?(int(*)(int,char*))dlsym(lib,"sceLncUtilGetAppTitleId"):nullptr;if(!find||!gettitle)return 1;int id=find(q.title);char observed[256]{};bool running=id>0&&gettitle(id,observed)==0&&!strcmp(observed,q.title);
 if(q.action==6){printf("APP STATE running=%d id=%d\n",running,id);return 0;}
 if(q.action==2){void* us=dlopen("libSceUserService.sprx",RTLD_LAZY);auto init=us?(int(*)(void*))dlsym(us,"sceUserServiceInitialize"):nullptr;auto fg=us?(int(*)(int*))dlsym(us,"sceUserServiceGetForegroundUser"):nullptr;auto launch=(int(*)(const char*,const char**,Param*))dlsym(lib,"sceSystemServiceLaunchApp");if(!init||!fg||!launch)return 1;int priority=256;init(&priority);int uid=-1;if(fg(&uid)||uid<=0)return 1;Param param{sizeof(Param),uid,0,0,0};const char* args[]={nullptr};int rc=launch(q.title,args,&param);printf("APP LAUNCH result=%08x\n",rc);return rc<0?1:0;}
 if(q.action==3){auto kill=(int(*)(int))dlsym(lib,"sceLncUtilKillApp");if(!running||!kill){printf("APP CLOSE running=%d\n",running);return running?1:0;}int rc=kill(id);printf("APP CLOSE result=%08x\n",rc);return rc?1:0;}
 if(running){puts("APP ERROR running; close before update/remove");return 1;}
 if(q.action==4){std::string stage=std::string(path)+".devbridge-stage",backup=std::string(path)+".devbridge-backup-"+std::to_string(time(nullptr));if(!metadata(stage,q.title))return 1;struct stat current{};bool existing=lstat(path,&current)==0;if(existing&&!metadata(path,q.title))return 1;if(existing&&rename(path,backup.c_str()))return 1;if(rename(stage.c_str(),path)){if(existing)rename(backup.c_str(),path);puts("APP ERROR restored backup");return 1;}printf("APP UPDATED backup=%s\n",backup.c_str());return 0;}
 if(q.action==5){void* app=dlopen("/system/common/lib/libSceAppInstUtil.sprx",RTLD_LAZY);auto init=app?(int(*)())dlsym(app,"sceAppInstUtilInitialize"):nullptr;auto remove=app?(int(*)(const char*))dlsym(app,"sceAppInstUtilAppUnInstall"):nullptr;if(!init||!remove)return 1;int rc=init();if(!rc)rc=remove(q.title);if(rc){printf("APP REMOVE error=%08x\n",rc);return 1;}std::string trash=std::string(path)+".devbridge-trash-"+std::to_string(time(nullptr));if(rename(path,trash.c_str()))return 1;unlink(marker.c_str());printf("APP REMOVED files_retained=%s\n",trash.c_str());return 0;}return 1;}
