// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) Read one PNG only below the exact user-provided screenshot root.
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
static bool emit(const void* data,size_t size){size_t at=0;while(at<size){ssize_t n=write(STDOUT_FILENO,(const char*)data+at,size-at);if(n<=0)return false;at+=n;}return true;}
int main(){
 struct Request{uint32_t magic,version;char relative[768];} request{};
 size_t at=0;while(at<sizeof(request)){ssize_t n=read(STDIN_FILENO,(char*)&request+at,sizeof(request)-at);if(n<=0)return 1;at+=n;}
 if(request.magic!=0x53484f54||request.version!=1||!memchr(request.relative,0,sizeof(request.relative)))return 1;
 const char* name=request.relative;size_t length=strlen(name);
 if(!length || name[0]=='/' || strstr(name,"..") || strspn(name,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-/")!=length || length<4 || strcmp(name+length-4,".png"))return 1;
 const char* root="/user/av_contents/photo/";
 char path[1024],resolved[1024];if(snprintf(path,sizeof(path),"%s%s",root,name)>=(int)sizeof(path)||!realpath(path,resolved)||strncmp(root,resolved,strlen(root)))return 1;
 int fd=open(resolved,O_RDONLY|O_NOFOLLOW);if(fd<0)return 1;struct stat st{};
 if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size<8 || st.st_size>32*1024*1024){close(fd);return 1;}
 unsigned char magic[8];if(read(fd,magic,8)!=8 || memcmp(magic,"\x89PNG\r\n\x1a\n",8)){close(fd);return 1;}
 char header[80];int n=snprintf(header,sizeof(header),"JBSHOT1 %lld\n",(long long)st.st_size);if(!emit(header,n)||!emit(magic,8)){close(fd);return 1;}
 size_t left=st.st_size-8;char block[32768];while(left){ssize_t got=read(fd,block,left>sizeof(block)?sizeof(block):left);if(got<=0||!emit(block,got)){close(fd);return 1;}left-=got;}
 close(fd);return 0;
}