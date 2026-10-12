// (AI-assisted) Fixed-target Store update, staged CRC verification and rollback.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
struct AppInfo{uint32_t id;uint64_t unknown;char title[14];char rest[0x3c];};
extern "C" int sceKernelGetAppInfo(pid_t,AppInfo*);
static bool receive(void* ptr,size_t n){auto* b=(unsigned char*)ptr;while(n){ssize_t rc=read(0,b,n);if(rc<=0)return false;b+=rc;n-=rc;}return true;}
static bool regular(const std::string& p){struct stat s{};return lstat(p.c_str(),&s)==0&&S_ISREG(s.st_mode);}
static bool store_stopped(){int mib[]={1,14,8,0};size_t n=0;if(sysctl(mib,4,nullptr,&n,nullptr,0)||!n||n>8*1024*1024)return false;
 std::vector<unsigned char>b(n);if(sysctl(mib,4,b.data(),&n,nullptr,0))return false;
 for(size_t at=0;at+4<=n;){int len=0;memcpy(&len,b.data()+at,4);if(len<(int)(offsetof(kinfo_proc,ki_pid)+sizeof(pid_t))||(size_t)len>n-at)return false;kinfo_proc p{};memcpy(&p,b.data()+at,std::min(sizeof(p),(size_t)len));at+=len;AppInfo info{};if(sceKernelGetAppInfo(p.ki_pid,&info)==0&&!memcmp(info.title,"FAKE00666",9))return false;}return true;}
int main(){
 const std::string root="/mnt/usb0/homebrew/FAKE00666";const char* names[]={"eboot.bin","browser-close.elf","sce_sys/param.json"};
 struct stat dir{};if(lstat(root.c_str(),&dir)||!S_ISDIR(dir.st_mode)||!store_stopped()){puts("INSTALL refused: target unavailable or Store running");return 1;}
 FILE* old=fopen((root+"/sce_sys/param.json").c_str(),"r");char metadata[8192]{};if(!old)return 1;fread(metadata,1,sizeof(metadata)-1,old);fclose(old);
 if(!strstr(metadata,"FAKE00666")||!strstr(metadata,"01.000.100")){puts("INSTALL refused: unexpected installed version");return 1;}
 uint32_t header[2]{};if(!receive(header,sizeof(header))||header[0]!=0x53555044||header[1]!=3)return 1;
 const std::string stage="/mnt/usb0/homebrew/.devbridge-stage-"+std::to_string(getpid());
 const std::string backup="/mnt/usb0/homebrew/.devbridge-backup-FAKE00666-v100-"+std::to_string(getpid());
 if(mkdir(stage.c_str(),0700)||mkdir((stage+"/sce_sys").c_str(),0700))return 1;
 for(unsigned i=0;i<3;i++){
  uint64_t size=0;uint32_t expected=0;if(!receive(&size,8)||!receive(&expected,4)||!size||size>64*1024*1024)return 1;
  FILE* f=fopen((stage+"/"+names[i]).c_str(),"wb");if(!f)return 1;uint32_t crc=0xffffffffu;unsigned char block[65536];uint64_t remaining=size;
  while(remaining){size_t count=std::min<uint64_t>(remaining,sizeof(block));if(!receive(block,count)||fwrite(block,1,count,f)!=count){fclose(f);return 1;}
   for(size_t j=0;j<count;j++){crc^=block[j];for(int bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1)));}remaining-=count;
  }
  int flush=fflush(f);int sync=fsync(fileno(f));int close=fclose(f);if(flush||sync||close||(crc^0xffffffffu)!=expected){puts("INSTALL staging verification failed");return 1;}
  if(chmod((stage+"/"+names[i]).c_str(),0777))return 1;
  printf("INSTALL verified %s bytes=%llu crc=%08x\n",names[i],(unsigned long long)size,expected);fflush(stdout);
 }
 FILE* param=fopen((stage+"/sce_sys/param.json").c_str(),"r");char text[8192]{};if(!param)return 1;fread(text,1,sizeof(text)-1,param);fclose(param);
 if(!strstr(text,"FAKE00666")||!strstr(text,"01.000.101")||!store_stopped())return 1;
 for(auto name:names)if(!regular(root+"/"+name)||!regular(stage+"/"+name))return 1;
 if(mkdir(backup.c_str(),0700)||mkdir((backup+"/sce_sys").c_str(),0700))return 1;
 unsigned saved=0;for(;saved<3;saved++)if(rename((root+"/"+names[saved]).c_str(),(backup+"/"+names[saved]).c_str()))break;
 if(saved!=3){while(saved){--saved;rename((backup+"/"+names[saved]).c_str(),(root+"/"+names[saved]).c_str());}puts("INSTALL backup failed; restored");return 1;}
 unsigned applied=0;for(;applied<3;applied++)if(rename((stage+"/"+names[applied]).c_str(),(root+"/"+names[applied]).c_str()))break;
 if(applied!=3){for(unsigned i=0;i<applied;i++)rename((root+"/"+names[i]).c_str(),(stage+"/"+names[i]).c_str());for(unsigned i=0;i<3;i++)rename((backup+"/"+names[i]).c_str(),(root+"/"+names[i]).c_str());puts("INSTALL apply failed; restored");return 1;}
 rmdir((stage+"/sce_sys").c_str());rmdir(stage.c_str());sync();
 printf("INSTALL COMPLETE version=01.000.101 backup=%s\n",backup.c_str());fflush(stdout);return 0;
}
