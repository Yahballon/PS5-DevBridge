// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdio>
namespace store::browser_launch {
struct Api {int (*initialize)(void*)=nullptr;int (*open)(const char*,void*)=nullptr;int (*terminate)()=nullptr;};
inline int launch(const Api& api,const char* url,char* message,unsigned capacity){
 if(!api.initialize||!api.open||!api.terminate){std::snprintf(message,capacity,"Browser system-service exports unavailable");return -1;}
 const int initialized=api.initialize(nullptr);
 // UserService already-initialized code used by existing PS5 applications.
 if(initialized<0&&static_cast<unsigned>(initialized)!=0x80960003u){std::snprintf(message,capacity,"Browser user-service initialization failed: 0x%08x",static_cast<unsigned>(initialized));return initialized;}
 const int result=api.open(url,nullptr);
 if(initialized==0)api.terminate();
 std::snprintf(message,capacity,result==0?"PS5 system browser opened":"PS5 system browser launch failed: 0x%08x",static_cast<unsigned>(result));
 return result;
}
}
