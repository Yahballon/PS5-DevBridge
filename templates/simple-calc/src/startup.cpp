// (AI-assisted) Local startup marker required by the native CRT.
#include <cstdio>
extern "C" void jb_startup_mark(const char* message){std::printf("[PS5CLEAN] %s\n",message);}
