#pragma once
#include <cstdint>
#include <cstddef>
namespace memory {
constexpr uint32_t kRequest=0x314d4244,kReply=0x31524d44;
constexpr uint32_t kMaxRead=1024*1024,kMaxBody=48+32;
struct Request {uint32_t magic;uint16_t version,command;int32_t pid;uint32_t count;uint64_t cookie,address;uint32_t length,flags;uint64_t reserved;};
struct Reply {uint32_t magic;int32_t status;uint64_t cookie;uint32_t length,reserved;uint64_t address;};
static_assert(sizeof(Request)==48&&sizeof(Reply)==32);
inline bool user_range(uint64_t address,uint64_t length){return address>=0x10000&&length&&length<=kMaxRead&&address<0x800000000000ULL&&length<=0x800000000000ULL-address;}
inline bool valid(const Request& r,size_t size){
 if(r.magic!=kRequest||r.version!=1||r.reserved||r.count||r.command<1||r.command>4)return false;
 if(r.command==1)return size==48&&r.pid==0&&!r.cookie&&!r.address&&!r.length&&!r.flags;
 if(r.pid<=1||!r.cookie)return false;
 if(r.command==2)return size==48&&!r.address&&!r.length&&!r.flags;
 if(!user_range(r.address,r.length))return false;
 if(r.command==3)return size==48&&!r.flags;
 return r.length<=16&&r.flags==1&&size==48+2*r.length;
}
}
