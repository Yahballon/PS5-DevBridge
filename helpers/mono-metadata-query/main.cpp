// (AI-assisted) Metadata cache queries only; no managed method or window-close invocation.
#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vector>
#include <algorithm>
#include <ps5/kernel.h>
static pid_t target_pid=-1;
static void note(const char* label,int rc){if(rc!=0)printf("MONO-META %s rc=%d\n",label,rc);}
#include "remote_query.hpp"
int main(){
 // (AI-assisted) Disabled after live scene enumeration restarted ShellUI.
 // Managed getters may execute UI code; a read-only intent does not make them safe.
 puts("MONO-META DISABLED: scene query caused a ShellUI restart; no remote calls executed");
 return 1;
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 int mib[]={1,14,8,0};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)<0||!size||size>8*1024*1024)return 1;
 std::vector<unsigned char> bytes(size);if(sysctl(mib,4,bytes.data(),&size,nullptr,0)<0)return 1;
 for(size_t at=0;at+4<=size;){int len=0;memcpy(&len,bytes.data()+at,4);if(len<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||(size_t)len>size-at)return 1;
 kinfo_proc p{};memcpy(&p,bytes.data()+at,std::min(sizeof(p),(size_t)len));at+=len;p.ki_comm[sizeof(p.ki_comm)-1]=0;if(!strcmp(p.ki_comm,"SceShellUI"))target_pid=p.ki_pid;
 }
 if(target_pid<=0)return 1;
 uint64_t thread=remote_query("mono_thread_current");printf("MONO-META thread=%llx\n",(unsigned long long)thread);if(!thread||thread==UINT64_MAX)return 1;
 uint64_t image=remote_query("mono_image_loaded","Sce.Vsh.ShellUI.ReactNativeApps");printf("MONO-META image_short=%llx\n",(unsigned long long)image);
 if(!image)image=remote_query("mono_image_loaded","Sce.Vsh.ShellUI.ReactNativeApps.dll");printf("MONO-META image=%llx pid=%d\n",(unsigned long long)image,target_pid);
 if(!image||image==UINT64_MAX)return 1;
 auto get_method=[&](uint32_t token,const char* expected){uint64_t m=remote_query("mono_get_method",nullptr,image,token,0);if(!m||m==UINT64_MAX)return uint64_t(0);uint64_t n=remote_query("mono_method_get_name",nullptr,m);char text[128]{};if(!n||n==UINT64_MAX||kernel_proc_copyout(target_pid,n,text,sizeof(text))!=0||strcmp(text,expected))return uint64_t(0);return m;};
 uint64_t getter=get_method(0x06000154,"get_Instance");if(!getter)return 1;
 uint64_t manager=remote_query("mono_runtime_invoke",nullptr,getter,0,0,0,true);printf("MONO-META control_center_manager=%llx\n",(unsigned long long)manager);
 if(!manager||manager==UINT64_MAX)return 1;
 for(auto token:{0x06000156u,0x0600015fu}){
  const char* name=token==0x06000156u?"get_UserId":"IsOpened";
  uint64_t method=get_method(token,name);if(!method)return 1;
  uint64_t box=remote_query("mono_runtime_invoke",nullptr,method,manager,0,0,true);
  if(!box||box==UINT64_MAX)return 1;
  uint64_t ptr=remote_query("mono_object_unbox",nullptr,box);uint32_t value=0;
  size_t count=token==0x06000156u?4:1;
  if(!ptr||ptr==UINT64_MAX||kernel_proc_copyout(target_pid,ptr,&value,count)!=0)return 1;
  printf("MONO-META %s=%u\n",name,value);
 }
 uint64_t sceneMethod=get_method(0x06000157u,"get_Scene");if(!sceneMethod)return 1;
 uint64_t scene=remote_query("mono_runtime_invoke",nullptr,sceneMethod,manager,0,0,true);
 printf("MONO-META scene=%llx\n",(unsigned long long)scene);
 if(scene&&scene!=UINT64_MAX){
  image=remote_query("mono_image_loaded","ReactNative.Vsh.Common");
  uint64_t currentMethod=get_method(0x06000076u,"get_CurrentParam");
  uint64_t current=currentMethod?remote_query("mono_runtime_invoke",nullptr,currentMethod,scene,0,0,true):0;
  printf("MONO-META current_param=%llx\n",(unsigned long long)current);
  if(current&&current!=UINT64_MAX){
   uint64_t configMethod=get_method(0x06000191u,"get_UrlConfig");
   uint64_t config=configMethod?remote_query("mono_runtime_invoke",nullptr,configMethod,current,0,0,true):0;
   if(config&&config!=UINT64_MAX){
    uint64_t str=remote_query("mono_object_to_string",nullptr,config,0);
    uint64_t utf=(str&&str!=UINT64_MAX)?remote_query("mono_string_to_utf8",nullptr,str):0;
    if(utf&&utf!=UINT64_MAX){
     char json[2048]{};
     if(kernel_proc_copyout(target_pid,utf,json,sizeof(json)-1)==0)printf("MONO-META view_config=%s\n",json);
     remote_query("mono_free",nullptr,utf);
    }
    uint64_t cc=remote_query("mono_object_get_class",nullptr,config);
    for(const char* api:{"mono_class_get_namespace","mono_class_get_name"}){
     uint64_t pp=remote_query(api,nullptr,cc);char nn[256]{};
     if(pp&&pp!=UINT64_MAX&&kernel_proc_copyout(target_pid,pp,nn,sizeof(nn)-1)==0)printf("MONO-META config_%s=%s\n",api,nn);
    }
   }
   uint64_t c=remote_query("mono_object_get_class",nullptr,current);
   uint64_t p=remote_query("mono_class_get_name",nullptr,c);char n[256]{};
   if(p&&p!=UINT64_MAX&&kernel_proc_copyout(target_pid,p,n,sizeof(n)-1)==0)printf("MONO-META param_class=%s\n",n);
  }
  uint64_t klass=remote_query("mono_object_get_class",nullptr,scene);
  for(const char* api:{"mono_class_get_namespace","mono_class_get_name"}){
   uint64_t ptr=remote_query(api,nullptr,klass);char name[256]{};
   if(ptr&&ptr!=UINT64_MAX&&kernel_proc_copyout(target_pid,ptr,name,sizeof(name)-1)==0)printf("MONO-META %s=%s\n",api,name);
  }
 }
 image=remote_query("mono_image_loaded","Sce.PlayStation.PUI");
 uint64_t sm=get_method(0x06000a9du,"get_Instance");
 uint64_t store=sm?remote_query("mono_runtime_invoke",nullptr,sm,0,0,0,true):0;
 if(store&&store!=UINT64_MAX){
  uint64_t method=get_method(0x06000aadu,"get_SceneLayers");
  uint64_t list=method?remote_query("mono_runtime_invoke",nullptr,method,store,0,0,true):0;
  if(list&&list!=UINT64_MAX){
   uint64_t cls=remote_query("mono_object_get_class",nullptr,list);
   auto offset=[&](const char* name){uint64_t f=remote_query("mono_class_get_field_from_name",name,cls,0,0,0,false,true);return f&&f!=UINT64_MAX?remote_query("mono_field_get_offset",nullptr,f):UINT64_MAX;};
   uint64_t io=offset("_items"),so=offset("_size"),array=0;int count=0;
   if(io<1024&&so<1024&&kernel_proc_copyout(target_pid,list+io,&array,8)==0&&kernel_proc_copyout(target_pid,list+so,&count,4)==0&&count>=0&&count<=32){
    uint64_t length=0;if(kernel_proc_copyout(target_pid,array+24,&length,8)==0&&length>=unsigned(count)&&length<=4096)
    for(int i=0;i<count;i++){uint64_t obj=0;if(kernel_proc_copyout(target_pid,array+32+8*i,&obj,8)!=0||!obj)continue;
     uint64_t cc=remote_query("mono_object_get_class",nullptr,obj),p=remote_query("mono_class_get_name",nullptr,cc);char n[256]{};
     if(p&&p!=UINT64_MAX&&kernel_proc_copyout(target_pid,p,n,sizeof(n)-1)==0)printf("MONO-META layer_%d=%llx class=%s\n",i,(unsigned long long)obj,n);
    }
   }
  }
 }
 if(store&&store!=UINT64_MAX)for(auto token:{0x06000aa4u,0x06000aa6u,0x06000aa8u}){
  const char* name=token==0x06000aa4u?"get_RootScene":token==0x06000aa6u?"GetTopParentSceneForModal":"GetScene";
  uint64_t method=get_method(token,name);
  uint64_t obj=method?remote_query("mono_runtime_invoke",nullptr,method,store,0,0,true):0;
  if(obj&&obj!=UINT64_MAX){uint64_t cls=remote_query("mono_object_get_class",nullptr,obj);uint64_t ptr=remote_query("mono_class_get_name",nullptr,cls);char n[256]{};
   if(ptr&&ptr!=UINT64_MAX&&kernel_proc_copyout(target_pid,ptr,n,sizeof(n)-1)==0)printf("MONO-META %s=%llx class=%s\n",name,(unsigned long long)obj,n);
   if(!strcmp(n,"RootScene")){
    uint64_t parent=cls;for(int depth=0;depth<6&&parent&&parent!=UINT64_MAX;depth++){
     uint64_t p=remote_query("mono_class_get_name",nullptr,parent);char text[256]{};
     if(p&&p!=UINT64_MAX&&kernel_proc_copyout(target_pid,p,text,sizeof(text)-1)==0)printf("MONO-META root_parent_%d=%s\n",depth,text);
     parent=remote_query("mono_class_get_parent",nullptr,parent);
    }
   }
   if(!strcmp(n,"RootScene"))for(auto tt:{0x06001ef6u,0x06001ebdu,0x06001ebcu}){
    const char* label=tt==0x06001ef6u?"get_CustomFrontFocusableScene":tt==0x06001ebdu?"get_SubScenes":"get_ModalSceneList";
    uint64_t mm=get_method(tt,label),oo=mm?remote_query("mono_runtime_invoke",nullptr,mm,obj,0,0,true):0;
    if(oo&&oo!=UINT64_MAX){uint64_t cc=remote_query("mono_object_get_class",nullptr,oo),pp=remote_query("mono_class_get_name",nullptr,cc);char nn[256]{};
     if(pp&&pp!=UINT64_MAX&&kernel_proc_copyout(target_pid,pp,nn,sizeof(nn)-1)==0)printf("MONO-META root_%s=%llx class=%s\n",label,(unsigned long long)oo,nn);
     if(!strcmp(nn,"ReadOnlyCollection`1")){
      uint64_t f=remote_query("mono_class_get_field_from_name","list",cc,0,0,0,false,true);
      uint64_t off=f&&f!=UINT64_MAX?remote_query("mono_field_get_offset",nullptr,f):UINT64_MAX;
      uint64_t backing=0;if(off<1024&&kernel_proc_copyout(target_pid,oo+off,&backing,8)==0&&backing){
       oo=backing;cc=remote_query("mono_object_get_class",nullptr,oo);pp=remote_query("mono_class_get_name",nullptr,cc);
       if(pp&&pp!=UINT64_MAX)kernel_proc_copyout(target_pid,pp,nn,sizeof(nn)-1);
      }
     }
     if(!strcmp(nn,"List`1")){
      auto offset=[&](const char* field){uint64_t f=remote_query("mono_class_get_field_from_name",field,cc,0,0,0,false,true);return f&&f!=UINT64_MAX?remote_query("mono_field_get_offset",nullptr,f):UINT64_MAX;};
      uint64_t io=offset("_items"),so=offset("_size"),arr=0;int count=0;
      if(io<1024&&so<1024&&kernel_proc_copyout(target_pid,oo+io,&arr,8)==0&&kernel_proc_copyout(target_pid,oo+so,&count,4)==0&&count>=0&&count<=32){
       for(int j=0;j<count;j++){uint64_t child=0;if(kernel_proc_copyout(target_pid,arr+32+8*j,&child,8)!=0||!child)continue;
        uint64_t k=remote_query("mono_object_get_class",nullptr,child),p=remote_query("mono_class_get_name",nullptr,k);char text[256]{};
        if(p&&p!=UINT64_MAX&&kernel_proc_copyout(target_pid,p,text,sizeof(text)-1)==0)printf("MONO-META root_child_%d=%llx class=%s\n",j,(unsigned long long)child,text);
       }
      }
     }
    }
   }
   if(!strcmp(n,"TransitionScene")){
    uint64_t mm=get_method(0x06001faau,"get_CurrentScene");
    uint64_t oo=mm?remote_query("mono_runtime_invoke",nullptr,mm,obj,0,0,true):0;
    if(oo&&oo!=UINT64_MAX){uint64_t cc=remote_query("mono_object_get_class",nullptr,oo);uint64_t pp=remote_query("mono_class_get_name",nullptr,cc);char nn[256]{};
     if(pp&&pp!=UINT64_MAX&&kernel_proc_copyout(target_pid,pp,nn,sizeof(nn)-1)==0)printf("MONO-META transition_current=%llx class=%s\n",(unsigned long long)oo,nn);
    }
   }
  }
 }
 return 0;
}
