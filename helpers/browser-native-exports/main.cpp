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

static void inspect(pid_t pid){
 uintptr_t proc=kernel_get_proc(pid),linker=0,obj=0;
 if(!kernel_pointer(proc)||kernel_copyout(proc+0x3e8,&linker,8)||!kernel_pointer(linker)||kernel_copyout(linker,&obj,8))return;
 std::vector<uintptr_t> seen;
 for(unsigned i=0;i<256&&kernel_pointer(obj);i++){
  if(std::find(seen.begin(),seen.end(),obj)!=seen.end())break;seen.push_back(obj);
  uintptr_t next=0,np=0;char name[1024]{};uint64_t handle=0;
  if(kernel_copyout(obj,&next,8)||kernel_copyout(obj+8,&np,8)||!kernel_pointer(np)||kernel_copyout(np,name,sizeof(name))||kernel_copyout(obj+0x28,&handle,8))break;
  name[1023]=0;
  if(strstr(name,"libSceNKWeb.sprx")||strstr(name,"libSceShellUIUtil.sprx")){
   uintptr_t info=0,base=0;unsigned char data[0x120]{};
   if(kernel_copyout(obj+0x148,&info,8)==0&&kernel_pointer(info)&&kernel_copyout(info,data,sizeof(data))==0&&kernel_copyout(obj+0x30,&base,8)==0&&base==(uintptr_t)kernel_dynlib_mapbase_addr(pid,(uint32_t)handle)){
    uintptr_t sym=0,str=0;uint64_t symsize=0,strsize=0;
    memcpy(&sym,data+0x28,8);memcpy(&symsize,data+0x30,8);memcpy(&str,data+0x38,8);memcpy(&strsize,data+0x40,8);
    if(kernel_pointer(sym)&&kernel_pointer(str)&&symsize&&symsize<=2*1024*1024&&symsize%24==0&&strsize&&strsize<=2*1024*1024){
     std::vector<unsigned char> symbols(symsize),strings(strsize);
     if(kernel_copyout(sym,symbols.data(),symsize)==0&&kernel_copyout(str,strings.data(),strsize)==0){
      printf("NATIVE-EXPORT module=%s pid=%d base=%llx count=%llu\n",name,pid,(unsigned long long)base,(unsigned long long)(symsize/24));
      for(size_t j=0;j<symsize;j+=24){uint32_t index=0;uint64_t value=0;memcpy(&index,symbols.data()+j,4);memcpy(&value,symbols.data()+j+8,8);
       if(value&&index<strsize&&memchr(strings.data()+index,0,std::min(size_t(256),size_t(strsize-index)))){printf("SYMBOL %s offset=%llx\n",strings.data()+index,(unsigned long long)value); unsigned char code[256]{}; if(value>=0x12af0&&value<=0x12ca0&&kernel_proc_copyout(pid,base+value,code,sizeof(code))==0){printf("NATIVE-CODE %llx ",(unsigned long long)value);for(auto b:code)printf("%02x",b);puts("");}}
      }
     }
    }else puts("NATIVE-EXPORT table bounds refused");
   }else puts("NATIVE-EXPORT layout refused");
  }
  obj=next;
 }
}
int main(){
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 int mib[]={1,14,8,0};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)<0||!size||size>8*1024*1024)return 1;
 std::vector<unsigned char> bytes(size);if(sysctl(mib,4,bytes.data(),&size,nullptr,0)<0)return 1;
 for(size_t at=0;at+4<=size;){int len=0;memcpy(&len,bytes.data()+at,4);if(len<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||(size_t)len>size-at)return 1;
 kinfo_proc p{};memcpy(&p,bytes.data()+at,std::min(sizeof(p),(size_t)len));at+=len;p.ki_comm[sizeof(p.ki_comm)-1]=0;if(!strcmp(p.ki_comm,"SceShellUI"))inspect(p.ki_pid);
 }
 puts("NATIVE-EXPORT done; no target calls or writes");return 0;
}
