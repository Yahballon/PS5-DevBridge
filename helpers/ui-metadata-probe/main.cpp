// (AI-assisted) Bounded CLI metadata extraction from specifically selected UI assemblies.
// No method invocation, input injection, credentials changes or memory writes.
#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <ps5/kernel.h>
static uint16_t u16(const unsigned char* p){uint16_t x;memcpy(&x,p,2);return x;}
static uint32_t u32(const unsigned char* p){uint32_t x;memcpy(&x,p,4);return x;}
static void inspect(pid_t pid,const char* name){
 uint32_t handle=0;if(kernel_dynlib_handle(pid,name,&handle)!=0)return;
 uintptr_t base=kernel_dynlib_mapbase_addr(pid,handle),init=kernel_dynlib_init_addr(pid,handle);unsigned char code[64]{};
 if(!base||!init||kernel_proc_copyout(pid,init,code,sizeof(code))!=0||memcmp(code+0x1e,"\x48\x3b\x1d",3))return;
 int32_t disp;memcpy(&disp,code+0x21,4);uintptr_t page=(init+0x25+disp)&~uintptr_t(0x3fff);
 if(page<base||page-base>64*1024*1024)return;
 std::vector<unsigned char> first(0x4000);if(kernel_proc_copyout(pid,page,first.data(),first.size())!=0)return;
 size_t mz=first.size();for(size_t i=0;i+64<first.size();i++)if(first[i]=='M'&&first[i+1]=='Z'){mz=i;break;}if(mz==first.size())return;
 const unsigned char* pe=first.data()+mz;size_t available=first.size()-mz;uint32_t pos=u32(pe+0x3c);
 if(pos+24>=available||memcmp(pe+pos,"PE\0\0",4))return;
 uint16_t count=u16(pe+pos+6),optional=u16(pe+pos+20);size_t opt=pos+24,sections=opt+optional;if(!count||count>64||sections+count*40>available)return;
 uint16_t magic=u16(pe+opt);size_t dirs=magic==0x10b?96:magic==0x20b?112:0;if(!dirs||opt+dirs+15*8>available)return;
 uint32_t cliRva=u32(pe+opt+dirs+14*8),fileSize=0;
 for(unsigned i=0;i<count;i++){auto section=pe+sections+i*40;uint32_t raw=u32(section+20),size=u32(section+16);if(raw>8*1024*1024||size>8*1024*1024-raw)return;fileSize=std::max(fileSize,raw+size);}
 auto rawOffset=[&](uint32_t rva)->uint32_t{for(unsigned i=0;i<count;i++){auto s=pe+sections+i*40;uint32_t va=u32(s+12),size=u32(s+16);if(rva>=va&&rva-va<size)return u32(s+20)+rva-va;}return 0xffffffffu;};
 uint32_t cli=rawOffset(cliRva);unsigned char header[24]{};if(cli==0xffffffffu||cli>fileSize||sizeof(header)>fileSize-cli||kernel_proc_copyout(pid,page+mz+cli,header,sizeof(header))!=0)return;
 uint32_t meta=rawOffset(u32(header+8)),size=u32(header+12);if(meta==0xffffffffu||size<32||size>4*1024*1024||meta>fileSize||size>fileSize-meta)return;
 std::vector<unsigned char> metadata(size);if(kernel_proc_copyout(pid,page+mz+meta,metadata.data(),metadata.size())!=0||memcmp(metadata.data(),"BSJB",4))return;
 if(!strcmp(name,"Sce.Vsh.ShellUI.AppSystem.dll.sprx"))for(uint32_t rva:{0x1233a,0x12434,0x12484,0x12efa,0x12f26}){
  uint32_t raw=rawOffset(rva);unsigned char il[128]{};if(raw!=0xffffffffu&&raw<fileSize&&sizeof(il)<=fileSize-raw&&kernel_proc_copyout(pid,page+mz+raw,il,sizeof(il))==0){printf("UI-METHOD-IL %s rva=%x ",name,rva);for(auto byte:il)printf("%02x",byte);puts("");}
 }
 printf("UI-CLI %s %u ",name,size);for(auto b:metadata)printf("%02x",b);puts("");
}
int main(){
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 int mib[]={1,14,8,0};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)<0||!size||size>8*1024*1024)return 1;
 std::vector<unsigned char> bytes(size);if(sysctl(mib,4,bytes.data(),&size,nullptr,0)<0)return 1;
 for(size_t at=0;at+4<=size;){int len=0;memcpy(&len,bytes.data()+at,4);if(len<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||(size_t)len>size-at)return 1;
 kinfo_proc p{};memcpy(&p,bytes.data()+at,std::min(sizeof(p),(size_t)len));at+=len;p.ki_comm[sizeof(p.ki_comm)-1]=0;if(strcmp(p.ki_comm,"SceShellUI"))continue;
 for(const char* name:{"Sce.Vsh.dll.sprx","Sce.PlayStation.PUI.dll.sprx","ReactNative.Components.Vsh.dll.sprx"})inspect(p.ki_pid,name);
 }
 puts("UI-CLI DONE");return 0;
}
