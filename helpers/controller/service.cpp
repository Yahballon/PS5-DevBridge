// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) Resident input worker; virtual devices remain bounded to each command.
#define main controller_command
#include "main.cpp"
#undef main
#include "../../loader/bridge_config.h"
#include <sys/socket.h>
#include <netinet/in.h>
static bool full_read(int fd,void* dst,size_t n){char* p=(char*)dst;while(n){auto got=read(fd,p,n);if(got<=0)return false;p+=got;n-=got;}return true;}
int main(){
 signal(SIGPIPE,SIG_IGN);
 int listener=socket(AF_INET,SOCK_STREAM,0);if(listener<0)return 1;
 sockaddr_in addr{};addr.sin_len=sizeof(addr);addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);addr.sin_port=htons(1343);
 if(bind(listener,(sockaddr*)&addr,sizeof(addr))||listen(listener,4)){close(listener);return 1;}
 for(;;){int fd=accept(listener,nullptr,nullptr);if(fd<0)continue;
  timeval timeout{5,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
  char header[80]{};uint32_t op=0,size=0;bool authenticated=full_read(fd,header,sizeof(header));
  unsigned diff=0;for(unsigned i=0;i<64;i++)diff|=(unsigned char)header[8+i]^(unsigned char)BRIDGE_KEY[i];
  memcpy(&op,header+72,4);memcpy(&size,header+76,4);
  if(!authenticated||memcmp(header,"PS5DEV1\0",8)||diff||op!=10||size<28||size>524){close(fd);continue;}
  unsigned char body[524]{};if(!full_read(fd,body,size)){close(fd);continue;}
  uint32_t count=0;memcpy(&count,body+8,4);
  if(count<1||count>32||size!=12+count*16){close(fd);continue;}
  int input[2];if(pipe(input)){close(fd);continue;}
  if(write(input[1],body,size)!=(ssize_t)size){close(input[0]);close(input[1]);close(fd);continue;}close(input[1]);
  // Reset per-sequence state while retaining the worker process.
  assigned=false;physical_logical=0;physical_restore_result=-1;foreground_user=-1;event_fd=-1;
  int saved_in=dup(0),saved_out=dup(1);dup2(input[0],0);close(input[0]);dup2(fd,1);
  controller_command();fflush(stdout);
  if(saved_in>=0){dup2(saved_in,0);close(saved_in);}else close(0);
  if(saved_out>=0){dup2(saved_out,1);close(saved_out);}else close(1);close(fd);
 }
}
