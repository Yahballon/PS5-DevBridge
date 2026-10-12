// (AI-assisted) One SIGTERM for the exact web-worker created by our browser probe.
// Never targets ShellUI, ShellCore, NKUI or a native app; no SIGKILL fallback.
#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <signal.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <ps5/kernel.h>
int main(){
 puts("WEB-WORKER test disabled: terminating the renderer does not close the window");return 2;
 if(kernel_get_fw_version()!=0x05100023u)return 1;
 int mib[]={1,14,8,0};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)<0||!size||size>8*1024*1024)return 1;
 std::vector<unsigned char> data(size);if(sysctl(mib,4,data.data(),&size,nullptr,0)<0)return 1;
 unsigned matches=0;pid_t target=-1;
 for(size_t at=0;at+4<=size;){int length=0;memcpy(&length,data.data()+at,4);if(length<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm))||(size_t)length>size-at)return 1;
 kinfo_proc p{};memcpy(&p,data.data()+at,std::min(sizeof(p),(size_t)length));at+=length;p.ki_comm[sizeof(p.ki_comm)-1]=0;
 if(!strcmp(p.ki_comm,"SceNKWebProcess")){matches++;if(p.ki_pid==161&&p.ki_ppid==51)target=p.ki_pid;}
 }
 if(matches!=1||target!=161){puts("WEB-WORKER refused: probe-created process is no longer the unique web-worker");return 1;}
 printf("WEB-WORKER one SIGTERM target_pid=%d name=SceNKWebProcess\n",target);fflush(stdout);
 int rc=kill(target,SIGTERM);printf("WEB-WORKER signal_rc=%d\n",rc);return rc==0?0:1;
}
