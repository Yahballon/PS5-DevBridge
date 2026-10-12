// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) Bounded read-only inventory below the exact screenshot root supplied by the user.
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
static unsigned files=0,dirs=0;
static void walk(const char* root,const char* relative,unsigned depth){
 if(depth>5 || files>=512 || dirs>=256)return;char folder[1024];snprintf(folder,sizeof(folder),"%s%s%s",root,*relative?"/":"",relative);
 DIR* dir=opendir(folder);if(!dir)return;dirs++;
 while(auto* item=readdir(dir)){
  if(item->d_name[0]=='.'||strchr(item->d_name,'/'))continue;
  char path[1024],name[768];int n=snprintf(name,sizeof(name),"%s%s%s",relative,*relative?"/":"",item->d_name);if(n<0||n>=(int)sizeof(name))continue;
  n=snprintf(path,sizeof(path),"%s/%s",root,name);if(n<0||n>=(int)sizeof(path))continue;
  struct stat st{};if(lstat(path,&st)!=0)continue;
  if(S_ISDIR(st.st_mode)){walk(root,name,depth+1);continue;}
  if(!S_ISREG(st.st_mode))continue;
  unsigned char magic[16]{};int fd=open(path,O_RDONLY|O_NOFOLLOW);ssize_t amount=fd<0?-1:read(fd,magic,sizeof(magic));if(fd>=0)close(fd);
  printf("PHOTO %s bytes=%lld mtime=%lld header=",name,(long long)st.st_size,(long long)st.st_mtime);
  if(amount>0)for(int i=0;i<amount;i++)printf("%02x",magic[i]);printf("\n");if(++files>=512)break;
 }
 closedir(dir);
}
int main(){setvbuf(stdout,nullptr,_IONBF,0);walk("/user/av_contents/photo","",0);printf("[photo-inventory] files=%u dirs=%u (AI-assisted); no image downloaded or modified\n",files,dirs);return 0;}