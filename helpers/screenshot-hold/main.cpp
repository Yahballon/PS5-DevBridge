// SPDX-License-Identifier: GPL-3.0-or-later
#include "one_shot.hpp"
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ps5/kernel.h>
#include <ps5/klog.h>
#include <cstdio>
#include <cerrno>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <signal.h>
#include <sys/sysctl.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cstdlib>
static void note(const char *text, int result)
{
    klog_printf("[browser-close-test] %s: %d / 0x%08x (errno=%d)\n", text, result,
                static_cast<unsigned>(result), errno);
    mkdir("/data/jb-store", 0755);
    mkdir("/data/jb-store/log", 0755);
    if (FILE *file = fopen("/data/jb-store/log/screenshot-buttons.log", "a"))
    {
        fprintf(file, "[browser-close-test] %s: %d / 0x%08x (errno=%d)\n", text, result,
                static_cast<unsigned>(result), errno);
        fclose(file);
    }
}
static int foreground_user=-1;
static int (*bind_device)(std::uint64_t,std::uint32_t)=nullptr;
static bool assigned=false;
static bool device_info(int fd,std::uint32_t id,unsigned char* output) {
 struct Info {std::uint8_t cmd=0,pad[3]{};std::uint32_t id=0;std::uint64_t reserved=0;void* out=nullptr;};
 Info info;info.id=id;info.out=output;return ioctl(fd,0xc0184852UL,&info)==0;
}
#include "remote_bind.hpp"
static int checked_bind(std::uint64_t id,std::uint32_t user){return remote_bind(id,user);}
static int event_fd=-1;
static std::uint64_t capture_mbus_id(std::uint32_t handle) {
 if(event_fd<0)return 0;std::string pending;char block[2048];size_t total=0;
 for(unsigned tick=0;tick<200 && total<256*1024;tick++) {
  ssize_t count=read(event_fd,block,sizeof(block));
  if(count>0){total+=count;pending.append(block,count);size_t end;
   while((end=pending.find('\n'))!=std::string::npos){std::string line=pending.substr(0,end);pending.erase(0,end+1);
    if(line.find("DEVICE_ADDED")==std::string::npos || line.find("subType:22")==std::string::npos || line.find("capabilityBattery:0")==std::string::npos)continue;
    size_t at=line.find("DeviceId:0x");if(at==std::string::npos)at=line.find("deviceId=0x");if(at==std::string::npos)continue;
    const auto id=std::strtoull(line.c_str()+at+11,nullptr,16);if((std::uint32_t)id==handle)return id;
   }
   if(pending.size()>8192)pending.clear();
  } else if(count<0 && errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR)break;
  usleep(10000);
 }
 return 0;
}
static int assign_virtual(int fd,std::uint32_t handle) {
 std::vector<unsigned char> info(65536,0);
 if(!device_info(fd,handle,info.data())) {note("Virtual device info failed",-1);return -1;}
 std::uint32_t type=0;std::uint64_t id=0;std::memcpy(&type,info.data(),4);std::memcpy(&id,info.data()+8,8);
 note("Virtual info type",type);note("Virtual Mbus ID low",(std::uint32_t)id);note("Virtual Mbus ID high",(std::uint32_t)(id>>32));
 std::uint32_t infohandle=0;std::memcpy(&infohandle,info.data()+4,4);note("Virtual info handle",infohandle);
 if(type!=1 || infohandle!=handle || (std::uint32_t)id!=handle || (id>>32)!=0x40000000u){note("Virtual identity mismatch",-1);return -1;}
 const auto event_id=capture_mbus_id(handle);note("System-event ID low",(std::uint32_t)event_id);note("System-event ID high",(std::uint32_t)(event_id>>32));
 if(!event_id){note("No matching system event: no bind or Circle",-1);return -1;}id=event_id;
 int rc=checked_bind(id,static_cast<std::uint32_t>(foreground_user));note("Direct virtual bind",rc);assigned=rc==0;return rc;
}
static std::uint32_t physical_logical=0;
static void capture_physical(){
 const pid_t pid=find_shellui();uint32_t pad=0;if(pid<0 || kernel_dynlib_handle(pid,"libScePad.sprx",&pad)!=0)return;
 uintptr_t fn=kernel_dynlib_dlsym(pid,pad,"scePadGetInfo");unsigned char code[256]{};
 if(!fn || kernel_proc_copyout(pid,fn,code,sizeof(code))!=0 || code[0xba]!=0x48 || code[0xbb]!=0x8d || code[0xbc]!=0x0d)return;
 int32_t displacement=0;std::memcpy(&displacement,code+0xbd,4);uintptr_t table=fn+0xc1+displacement;
 int fd=open("/dev/hid",O_RDONLY);if(fd<0)return;unsigned matches=0;
 for(unsigned i=0;i<16;i++){
  std::uint32_t id=0;if(kernel_proc_copyout(pid,table+0x30+i*0x548,&id,4)!=0 || !id)continue;
  std::vector<unsigned char> info(65536,0);if(!device_info(fd,id,info.data()))continue;
  std::uint16_t vendor=0,product=0;std::uint32_t returned=0;std::memcpy(&vendor,info.data()+16,2);std::memcpy(&product,info.data()+18,2);std::memcpy(&returned,info.data()+4,4);
  if(vendor==0x054c && product==0x0ce6 && returned==id){physical_logical=id;matches++;}
 }
 close(fd);if(matches!=1)physical_logical=0;note("Verified physical logical ID",physical_logical);
}
static void restore_physical() {
 if(!assigned || !physical_logical)return;
 int fd=open("/dev/hid",O_RDONLY);if(fd<0){note("Restore HID unavailable",-1);return;}
 std::vector<unsigned char> info(65536,0);bool read=device_info(fd,physical_logical,info.data());close(fd);
 std::uint16_t vid=0,pid=0;std::uint64_t id=0;std::memcpy(&vid,info.data()+16,2);std::memcpy(&pid,info.data()+18,2);std::uint32_t logical=0;std::memcpy(&logical,info.data()+4,4);id=logical;
 if(!read || logical!=physical_logical || vid!=0x054c || pid!=0x0ce6 || !id){note("Physical restore identity mismatch",-1);return;}
 note("Direct physical bind",checked_bind(id,static_cast<std::uint32_t>(foreground_user)));
}
int main()
{
    note("Starting controller held-Create screenshot test v2 (AI-assisted)",
         static_cast<int>(kernel_get_fw_version()));
    if(kernel_get_fw_version()!=0x05100023u) {note("Unsupported firmware: no input attempted",-1);return 1;}
    void *service = dlopen("libSceUserService.sprx", RTLD_LAZY);
    if (!service)
    {
        note("UserService unavailable", -1);
        return 1;
    }
    auto initialize = reinterpret_cast<int (*)(void *)>(dlsym(service, "sceUserServiceInitialize"));
    auto foreground =
        reinterpret_cast<int (*)(int *)>(dlsym(service, "sceUserServiceGetForegroundUser"));
    auto terminate = reinterpret_cast<int (*)()>(dlsym(service, "sceUserServiceTerminate"));
    if (!initialize || !foreground || !terminate)
    {
        note("UserService exports unavailable", -1);
        return 1;
    }
    int initialized = initialize(nullptr);
    if (initialized != 0 && static_cast<unsigned>(initialized) != 0x80960003u)
    {
        note("UserService initialize failed", initialized);
        return 1;
    }
    int user = -1;
    int status = foreground(&user);
    if (status != 0 || user < 0)
    {
        note("No foreground user", status);
        if (initialized == 0)
            terminate();
        return 1;
    }
    note("Foreground user", user);
    foreground_user=user;
    void* mbus=dlopen("libSceMbus.sprx",RTLD_LAZY);
    auto mi=mbus?reinterpret_cast<int(*)()>(dlsym(mbus,"sceMbusInit")):nullptr;
    auto mt=mbus?reinterpret_cast<int(*)()>(dlsym(mbus,"sceMbusTerm")):nullptr;
    bind_device=mbus?reinterpret_cast<int(*)(std::uint64_t,std::uint32_t)>(dlsym(mbus,"sceMbusBindDeviceWithUserId")):nullptr;
    if(!mi || !mt || !bind_device){note("Mbus exports unavailable",-1);if(initialized==0)terminate();return 1;}
    int mrc=mi();note("Mbus initialize",mrc);
    if(mrc!=0){if(initialized==0)terminate();return 1;}
    // Open the browser before launching this payload from a paired PC/phone. No repeated inputs.
    usleep(2000000);
    using namespace store::browser_close_test;
    Api api{+[] { return open("/dev/hid", O_RDWR); },
            +[](int fd, unsigned long command, void *request)
            { return ioctl(fd, command, request); },
            +[](int fd) { return close(fd); }, +[](unsigned ms) { usleep(ms * 1000); }, note};
    // Match the tested FGG-XSense descriptor value; retain the real foreground ID in the log.
    note("Descriptor selector", 1);
    event_fd=open("/dev/klog",O_RDONLY|O_NONBLOCK);note("System event reader",event_fd);
    if(event_fd>=0){char drain[4096];for(unsigned i=0;i<64;i++)if(read(event_fd,drain,sizeof(drain))<=0)break;}
    api.assign=assign_virtual;api.restore=restore_physical;
    capture_physical();
    int result = run(api, 1);
    if(event_fd>=0)close(event_fd);
    note("Mbus terminate",mt());
    note("Test complete", result);
    if (initialized == 0)
        terminate();
    return result == 0 ? 0 : 1;
}
