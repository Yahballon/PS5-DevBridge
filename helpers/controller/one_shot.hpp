// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) One-shot virtual pad report layout based on FGG-XSense research.
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <unistd.h>
namespace store::browser_close_test
{
constexpr unsigned long add_command = 0xC018482AUL, insert_command = 0x8010482CUL,
                        delete_command = 0x80104850UL;
struct Request
{
    std::uint32_t command = 0, reserved = 0;
    void *data = nullptr;
    void *output = nullptr;
};
// (AI-assisted) Firmware 5.10 dump: ioctl request at rsp+0x10, data at +0x18.
struct InsertRequest {std::uint32_t handle=0, reserved=0; void* data=nullptr;};
static_assert(sizeof(InsertRequest)==16 && offsetof(InsertRequest,data)==8);
struct Delete
{
    std::uint8_t command = 5;
    std::uint8_t padding[7]{};
    std::uint32_t handle = 0, reserved = 0;
};
static_assert(sizeof(Request) == 24 && sizeof(Delete) == 16);
struct Api
{
    int (*open_device)();
    int (*control)(int, unsigned long, void *);
    int (*close_device)(int);
    void (*sleep_ms)(unsigned);
    void (*log)(const char *, int);
    int (*assign)(int,std::uint32_t)=nullptr;
    void (*restore)()=nullptr;
};
struct Step {std::uint32_t buttons;std::uint16_t duration_ms,pause_ms;std::uint8_t lx,ly,rx,ry,l2,r2,reserved[2];};
static_assert(sizeof(Step)==16);
inline std::array<Step,32> steps{};
inline unsigned step_count=0;
inline int run(Api api, std::uint32_t user)
{
    int fd = api.open_device();
    if (fd < 0)
    {
        api.log("open /dev/hid failed", fd);
        return -1;
    }
    // Extra zero padding avoids exposing adjacent stack data if an older kernel copies a larger
    // descriptor/report.
    std::array<unsigned char, 256> descriptor{};
    descriptor[0] = 3;
    descriptor[0x20] = 1;
    std::memcpy(descriptor.data() + 0x1c, &user, 4);
    std::int32_t handle = -1;
    Request request;
    request.command = 3;
    request.data = descriptor.data();
    request.output = &handle;
    int result = api.control(fd, add_command, &request);
    api.log("AddDevice", result);
    if (result != 0 || handle < 0)
    {
        if (handle >= 0)
        {
            Delete remove;
            remove.handle = static_cast<std::uint32_t>(handle);
            int removed = api.control(fd, delete_command, &remove);
            api.log("DeleteDevice after AddDevice error", removed);
        }
        api.close_device(fd);
        return -2;
    }
    api.log("Created device handle", handle);
    std::array<unsigned char, 256> report{};
    for (unsigned i = 0x10; i < 0x14; ++i)
        report[i] = 128;
    report[0x17] = 1;
    float identity = 1.0f;
    std::memcpy(report.data() + 0x84, &identity, 4);
    InsertRequest insert;
    insert.handle=static_cast<std::uint32_t>(handle);
    insert.data=report.data();
    // (AI-assisted) Keep the device alive while ShellUI processes user assignment.
    bool okay = api.assign ? api.assign(fd,static_cast<std::uint32_t>(handle))==0 : true;
    for (unsigned i = 0; i < 120; ++i)
    {
        result = api.control(fd, insert_command, &insert);
        if (result != 0)
        {
            okay = false;
            break;
        }
        api.sleep_ms(16667 / 1000);
    }
    api.log("Neutral warm-up", okay ? 0 : result);
    // (AI-assisted) One Cross after assignment, followed by neutral settling and Circle.
    const std::uint32_t cross=0x4000, cross_release=0;
    if(okay){
      std::memcpy(report.data()+0x0c,&cross,4);
      for(unsigned n=0;n<6;n++){result=api.control(fd,insert_command,&insert);if(result!=0){okay=false;break;}api.sleep_ms(16);}
      api.log("Profile Cross press",result);
      std::memcpy(report.data()+0x0c,&cross_release,4);
      for(unsigned n=0;n<90;n++){result=api.control(fd,insert_command,&insert);if(result!=0){okay=false;break;}api.sleep_ms(16);}
      api.log("Profile Cross release",result);
    }
    auto frames=[&](unsigned duration,bool interruptible=true){
      for(unsigned ms=0;ms<duration;ms+=16){if(interruptible && access("/data/ps5-devbridge/stop-input",F_OK)==0){result=-6;return false;}result=api.control(fd,insert_command,&insert);if(result!=0)return false;api.sleep_ms(16);}return true;
    };
    for(unsigned i=0;i<step_count && okay;i++){
      const Step& step=steps[i];std::memcpy(report.data()+0x0c,&step.buttons,4);
      report[0x10]=step.lx;report[0x11]=step.ly;report[0x12]=step.rx;report[0x13]=step.ry;
      report[0x14]=step.l2;report[0x15]=step.r2;
      okay=frames(step.duration_ms);api.log("Command step",okay?(int)i:result);
      const std::uint32_t neutral=0;std::memcpy(report.data()+0x0c,&neutral,4);
      for(unsigned j=0x10;j<0x14;j++)report[j]=128;report[0x14]=report[0x15]=0;
      if(!frames(step.pause_ms))okay=false;
    }
    const std::uint32_t neutral=0;std::memcpy(report.data()+0x0c,&neutral,4);
    for(unsigned j=0x10;j<0x14;j++)report[j]=128;report[0x14]=report[0x15]=0;
    if(!frames(500,false))okay=false;api.log("Release all",result);
    Delete remove;
    remove.handle = static_cast<std::uint32_t>(handle);
    int removed = -1;
    for (unsigned i = 0; i < 3; ++i)
    {
        removed = api.control(fd, delete_command, &remove);
        if (removed == 0)
            break;
        api.sleep_ms(100);
    }
    api.log("DeleteDevice", removed);
    if(removed==0 && api.restore)api.restore();
    api.close_device(fd);
    return removed != 0 ? -3 : okay ? 0 : -4;
}
} // namespace store::browser_close_test
