// (AI-assisted) Read-only browser/UI module-name inventory and SDK lookup inspection.
#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <ps5/kernel.h>
static bool kernel_pointer(uintptr_t address){return (address>>48)==0xffff && !(address&7);}
static void list_modules(pid_t pid,const char* process){
 uintptr_t proc=kernel_get_proc(pid),linker=0,obj=0;
 if(!kernel_pointer(proc)||kernel_copyout(proc+0x3e8,&linker,8)!=0||!kernel_pointer(linker)||kernel_copyout(linker,&obj,8)!=0)return;
 std::vector<uintptr_t> seen;
 for(unsigned i=0;i<256&&kernel_pointer(obj);i++){
  if(std::find(seen.begin(),seen.end(),obj)!=seen.end())break;seen.push_back(obj);
  uintptr_t name=0,next=0;uint64_t handle=0;char path[1024]{};
  if(kernel_copyout(obj,&next,8)!=0||kernel_copyout(obj+8,&name,8)!=0||kernel_copyout(obj+0x28,&handle,8)!=0||!kernel_pointer(name)||kernel_copyout(name,path,sizeof(path))!=0)break;
  path[1023]=0;printf("MODULE-LIST pid=%d process=%s handle=%llu path=%.1023s\n",pid,process,(unsigned long long)handle,path);obj=next;
 }
}
int main(){
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 unsigned char layout[6]{};if(kernel_proc_copyout(getpid(),(uintptr_t)&kernel_dynlib_handle+0x2d,layout,6)!=0||memcmp(layout,"\x48\x05\xe8\x03\x00\x00",6)){puts("MODULE-LIST SDK layout guard refused");return 1;}

for(const char* path:{"/system_ex/common_ex/lib/Sce.Vsh.WebBrowser.dll","/system_ex/common_ex/lib/Sce.Vsh.WebBrowser.dll.sprx","/system/common_ex/lib/Sce.Vsh.WebBrowser.dll","/mnt/sandbox/NPXS40087_000/BSHE0dX0T0/common_ex/lib/Sce.Vsh.WebBrowser.dll","/mnt/sandbox/NPXS40087_000/BSHE0dX0T0/common_ex/lib/Sce.Vsh.WebBrowser.dll.sprx"}){
 struct stat st{};if(stat(path,&st)==0){unsigned char first[16]{};int fd=open(path,O_RDONLY);if(fd>=0){read(fd,first,16);close(fd);}printf("BROWSER-FILE path=%s bytes=%lld header=",path,(long long)st.st_size);for(auto b:first)printf("%02x",b);puts("");}
}
 unsigned char implementation[1024]{};uintptr_t own=(uintptr_t)&kernel_dynlib_handle;
 if(kernel_proc_copyout(getpid(),own,implementation,sizeof(implementation))==0){printf("SDK-LOOKUP-CODE ");for(auto b:implementation)printf("%02x",b);printf("\n");}
 int mib[]={1,14,8,0};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)<0||!size||size>8*1024*1024)return 1;
 std::vector<unsigned char> bytes(size);if(sysctl(mib,4,bytes.data(),&size,nullptr,0)<0)return 1;
 for(size_t at=0;at+4<=size;){int len=0;memcpy(&len,bytes.data()+at,4);if(len<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||(size_t)len>size-at)return 1;
 kinfo_proc p{};memcpy(&p,bytes.data()+at,std::min(sizeof(p),(size_t)len));at+=len;p.ki_comm[sizeof(p.ki_comm)-1]=0;
 if(strcmp(p.ki_comm,"SceShellUI")&&strcmp(p.ki_comm,"SceNKUIProcess")&&strcmp(p.ki_comm,"SceNKWebProcess"))continue;
 if(!strcmp(p.ki_comm,"SceShellUI")){
  uint32_t browser=0;if(kernel_dynlib_handle(p.ki_pid,"Sce.Vsh.WebBrowser.dll.sprx",&browser)==0){
   uintptr_t base=kernel_dynlib_mapbase_addr(p.ki_pid,browser),init=kernel_dynlib_init_addr(p.ki_pid,browser),entry=kernel_dynlib_entry_addr(p.ki_pid,browser);
   uintptr_t info=kernel_dynlib_dlsym(p.ki_pid,browser,"mono_aot_module_Sce_Vsh_WebBrowser_info");
   printf("BROWSER-ASSEMBLY pid=%d handle=%u base=%llx init=%llx entry=%llx info=%llx\n",p.ki_pid,browser,(unsigned long long)base,(unsigned long long)init,(unsigned long long)entry,(unsigned long long)info);
   if(base){
    std::vector<unsigned char> data(0xc000);unsigned pages=0;
    for(unsigned off=0;off<data.size();off+=0x4000)if(kernel_proc_copyout(p.ki_pid,base+off,data.data()+off,0x4000)==0)pages++;
    printf("BROWSER-CLI-DATA ");for(unsigned i=0x8000;i<0xc000;i++)printf("%02x",data[i]);puts("");
    printf("BROWSER-METADATA readable_pages=%u\n",pages);
    unsigned count=0;for(size_t i=0;i<data.size()&&count<160;){
     size_t start=i;while(i<data.size()&&data[i]>=32&&data[i]<=126)i++;size_t n=i-start;i++;
     if(n<4||n>180)continue;char str[181]{};memcpy(str,data.data()+start,n);
     if(strstr(str,"Browser")||strstr(str,"Close")||strstr(str,"Dispose")||strstr(str,"Cancel")||strstr(str,"Back")||strstr(str,"mono_")||strstr(str,"Web")){printf("BROWSER-META +%zx %s\n",start,str);count++;}
    }
   }
   for(auto address:{base,init,entry,info})if(address){unsigned char chunk[256]{};int rc=kernel_proc_copyout(p.ki_pid,address,chunk,sizeof(chunk));printf("BROWSER-CHUNK address=%llx rc=%d ",(unsigned long long)address,rc);if(rc==0)for(auto b:chunk)printf("%02x",b);puts("");}
  }
  uint32_t mono=0;if(kernel_dynlib_handle(p.ki_pid,"libmonosgen-2.0.sprx",&mono)==0)for(const char* name:{"mono_get_root_domain","mono_assembly_name_new","mono_assembly_loaded","mono_assembly_get_image","mono_image_get_name","mono_class_from_name","mono_class_get_methods","mono_method_get_name","mono_class_get_method_from_name","mono_runtime_invoke"})printf("MONO-API pid=%d name=%s address=%llx\n",p.ki_pid,name,(unsigned long long)kernel_dynlib_dlsym(p.ki_pid,mono,name));
 }
 if(strcmp(p.ki_comm,"SceNKWebProcess"))list_modules(p.ki_pid,p.ki_comm);
 for(const char* name:{"eboot.bin","NKUIProcess.self","NKWebProcess.self","libSceNKUI.sprx","libSceNKWeb.sprx","libSceWebKit2.sprx","libSceWebKit.sprx","libSceJSC.sprx","libSceNKit.sprx","libSceSystemService.sprx","libSceCommonDialog.sprx","libSceShellUIUtil.sprx"}){
  uint32_t handle=0;int rc=kernel_dynlib_handle(p.ki_pid,name,&handle);if(rc==0)printf("UI-MODULE pid=%d process=%s module=%s handle=%u base=%llx\n",p.ki_pid,p.ki_comm,name,handle,(unsigned long long)kernel_dynlib_mapbase_addr(p.ki_pid,handle));
 }
 }
 return 0;
}
