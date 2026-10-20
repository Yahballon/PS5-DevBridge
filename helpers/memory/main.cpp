// SPDX-License-Identifier: GPL-3.0-or-later
#include "protocol.hpp"
#include <ps5/kernel.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>
using memory::Request;
namespace {
bool exact(int fd,void* dst,size_t size){char* p=(char*)dst;while(size){auto n=read(fd,p,size);if(n<=0)return false;p+=n;size-=n;}return true;}
bool sendall(int fd,const void* src,size_t size){auto* p=(const char*)src;while(size){auto n=write(fd,p,size);if(n<=0)return false;p+=n;size-=n;}return true;}
void respond(int fd,int code,uint64_t cookie,uint64_t address,const void* data,size_t size){memory::Reply r{memory::kReply,code,cookie,uint32_t(size),0,address};if(sendall(fd,&r,sizeof(r))&&size)sendall(fd,data,size);}
void error(int fd,const Request&r,const char* text){respond(fd,-1,r.cookie,r.address,text,strlen(text));}
std::string quote(const char* text){std::string result="\"";for(size_t i=0;text[i]&&i<1024;i++){unsigned char c=text[i];if(c=='"'||c=='\\')result+='\\';if(c>=32&&c<127)result+=c;else result+='?';}return result+'"';}
std::vector<char> query(int kind,int pid,size_t maximum){int mib[]={CTL_KERN,KERN_PROC,kind,pid};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)||size>maximum)throw std::runtime_error("sysctl unavailable or oversized");size_t capacity=std::min(maximum,size+65536);std::vector<char> data(capacity);if(sysctl(mib,4,data.data(),&capacity,nullptr,0))throw std::runtime_error("sysctl changed; retry");data.resize(capacity);return data;}
std::vector<kinfo_proc> processes(){auto data=query(KERN_PROC_PROC,0,8*1024*1024);std::vector<kinfo_proc> result;for(size_t at=0;at+4<=data.size();){int n=0;memcpy(&n,data.data()+at,4);if(n<int(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||size_t(n)>data.size()-at)throw std::runtime_error("invalid process record");kinfo_proc p{};memcpy(&p,data.data()+at,std::min(sizeof(p),size_t(n)));at+=n;p.ki_comm[sizeof(p.ki_comm)-1]=0;result.push_back(p);}return result;}
uint64_t identity(const kinfo_proc&p){if(p.ki_start.tv_sec<=0)return 0;uint64_t h=14695981039346656037ULL;for(auto value:{uint64_t(p.ki_pid),uint64_t(p.ki_start.tv_sec),uint64_t(p.ki_start.tv_usec)})for(int i=0;i<8;i++){h^=(value>>(i*8))&255;h*=1099511628211ULL;}return h?h:1;}
bool eligible(const kinfo_proc&p){return p.ki_pid>1&&p.ki_pid!=getpid()&&(!strcmp(p.ki_comm,"eboot.bin")||!strcmp(p.ki_comm,"db-memory-lab"));}
bool target(const Request&r){for(const auto&p:processes())if(p.ki_pid==r.pid)return eligible(p)&&identity(p)==r.cookie;return false;}
struct Map {uint64_t start,end,offset;int protection,type;std::string path;};
std::vector<Map> maps(int pid){auto data=query(KERN_PROC_VMMAP,pid,32*1024*1024);std::vector<Map> result;for(size_t at=0;at+4<=data.size();){int n=0;memcpy(&n,data.data()+at,4);if(n<int(offsetof(kinfo_vmentry,kve_protection)+sizeof(int))||size_t(n)>data.size()-at)throw std::runtime_error("invalid map record");kinfo_vmentry v{};memcpy(&v,data.data()+at,std::min(sizeof(v),size_t(n)));at+=n;v.kve_path[sizeof(v.kve_path)-1]=0;if(v.kve_start<v.kve_end&&v.kve_end<=0x800000000000ULL)result.push_back({v.kve_start,v.kve_end,v.kve_offset,v.kve_protection,v.kve_type,v.kve_path});}return result;}
bool range(const Request&r,bool write){for(const auto&m:maps(r.pid))if(r.address>=m.start&&r.address<m.end&&r.length<=m.end-r.address){if(!(m.protection&1))return false;if(write&&(!(m.protection&2)||(m.protection&4)))return false;return m.type==1||m.type==2||m.type==9;}return false;}
void audit(const Request&r,const char* event){FILE*f=fopen("/data/ps5-devbridge/memory.log","ab");if(f){fprintf(f,"%s pid=%d cookie=%llx address=%llx bytes=%u\n",event,r.pid,(unsigned long long)r.cookie,(unsigned long long)r.address,r.length);fclose(f);}}
void command(int fd,const Request&r,const unsigned char* body){
 if(r.command==1){std::string json="[";bool comma=false;for(const auto&p:processes()){if(comma)json+=',';comma=true;json+="{\"pid\":"+std::to_string(p.ki_pid)+",\"name\":"+quote(p.ki_comm)+",\"cookie\":"+std::to_string(identity(p))+",\"eligible\":"+(eligible(p)&&identity(p)?"true":"false")+"}";}json+=']';respond(fd,0,0,0,json.data(),json.size());return;}
 if(!target(r)){error(fd,r,"Target missing, restarted or not an eligible app");return;}
 if(r.command==2){std::string json="[";bool comma=false;for(const auto&m:maps(r.pid)){if(comma)json+=',';comma=true;json+="{\"start\":"+std::to_string(m.start)+",\"end\":"+std::to_string(m.end)+",\"offset\":"+std::to_string(m.offset)+",\"protection\":"+std::to_string(m.protection)+",\"type\":"+std::to_string(m.type)+",\"path\":"+quote(m.path.c_str())+"}";if(json.size()>8*1024*1024)throw std::runtime_error("map response too large");}json+=']';respond(fd,0,r.cookie,0,json.data(),json.size());return;}
 if(!range(r,r.command==4)){error(fd,r,"Unmapped, device, protected or executable write range");return;}
 std::vector<unsigned char> data(r.length);
 if(kernel_proc_copyout(r.pid,r.address,data.data(),data.size())){error(fd,r,"Process read failed");return;}
 if(r.command==3){respond(fd,0,r.cookie,r.address,data.data(),data.size());return;}
 if(memcmp(data.data(),body,r.length)){audit(r,"COMPARE_REJECTED");error(fd,r,"Value changed before write; no write performed");return;}
 if(!target(r)){error(fd,r,"Process identity changed before write");return;}
 if(kernel_proc_copyin(r.pid,body+r.length,r.address,r.length)){audit(r,"WRITE_FAILED");error(fd,r,"Process write failed; inspect current value");return;}
 if(kernel_proc_copyout(r.pid,r.address,data.data(),data.size())||memcmp(data.data(),body+r.length,r.length)){audit(r,"VERIFY_FAILED");error(fd,r,"Write readback differs; target may be changing the value");return;}
 audit(r,"WRITE_VERIFIED");respond(fd,0,r.cookie,r.address,data.data(),data.size());
}
}
int main(){
 signal(SIGPIPE,SIG_IGN);syscall(SYS_thr_set_name,-1,"db-memory-worker");
 int listener=socket(AF_INET,SOCK_STREAM,0);if(listener<0)return 1;
 sockaddr_in address{};address.sin_len=sizeof(address);address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(1345);
 if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,8)){close(listener);return 1;}
 for(;;){int fd=accept(listener,nullptr,nullptr);if(fd<0)continue;timeval timeout{10,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
  char header[80]{};uint32_t op=0,size=0;
  if(!exact(fd,header,sizeof(header))||memcmp(header,"PS5DEV1\0",8)){close(fd);continue;}
  memcpy(&op,header+72,4);memcpy(&size,header+76,4);
  if(op!=21||size<48||size>memory::kMaxBody){close(fd);continue;}
  unsigned char body[memory::kMaxBody]{};
  if(!exact(fd,body,size)){close(fd);continue;}
  Request r{};memcpy(&r,body,sizeof(r));
  if(!memory::valid(r,size)){error(fd,r,"Invalid memory command");close(fd);continue;}
  try{command(fd,r,body+sizeof(r));}catch(const std::exception&e){error(fd,r,e.what());}catch(...){error(fd,r,"Memory worker exception");}
  close(fd);
 }
}
