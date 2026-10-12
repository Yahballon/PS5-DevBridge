// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) Bounded allowlisted diagnostics only, no uploads or system changes.
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
static bool emit(const char* p,size_t n){while(n){ssize_t rc=write(STDOUT_FILENO,p,n);if(rc<=0)return false;p+=rc;n-=rc;}return true;}
int main(){
 const char* names[]={"pad-diagnostic.log","pad-abi-probe.log","mbus-abi-probe.log","pad-device-probe.log","pad-state-probe.log","browser-close-test.log","browser-button-test.log","browser-handoff.log","browser-close.log","screenshot-buttons.log"};
 emit("JBLOG1 BEGIN\n",13);
 for(const char* name:names){
  char path[256],header[320];if(!strcmp(name,"browser-handoff.log"))snprintf(path,sizeof(path),"/data/jb-store/%s",name);else snprintf(path,sizeof(path),"/data/jb-store/log/%s",name);
  int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)continue;
  struct stat st{};if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size<0){close(fd);continue;}
  const size_t limit=256*1024;size_t amount=st.st_size>limit?limit:(size_t)st.st_size;
  if((size_t)st.st_size>limit && lseek(fd,st.st_size-limit,SEEK_SET)<0){close(fd);continue;}
  int length=snprintf(header,sizeof(header),"JBLOG1 FILE %s %zu\n",name,amount);
  if(!emit(header,length)){close(fd);return 1;}
  char block[4096];while(amount){ssize_t n=read(fd,block,amount>sizeof(block)?sizeof(block):amount);if(n<=0){close(fd);return 1;}if(!emit(block,n)){close(fd);return 1;}amount-=n;}
  close(fd);emit("\nJBLOG1 FILE-END\n",17);
 }
 emit("JBLOG1 DONE\n",12);return 0;
}