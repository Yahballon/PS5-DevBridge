// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) Bounded file chunks, staging and explicit commit.
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
struct FileCommand{char path[1024];uint64_t offset;uint32_t length,flags;};
static int allowed_path(const char* p){if(!memchr(p,0,1024)||strstr(p,"..")||strchr(p,'\n')||strchr(p,'\r'))return 0;return !strncmp(p,"/data/",6)||(!strncmp(p,"/mnt/usb",8)&&p[8]>='0'&&p[8]<='7'&&p[9]=='/');}
static int checked_path(const char* p,int creating){char resolved[PATH_MAX],parent[1024];if(!allowed_path(p))return 0;snprintf(parent,sizeof(parent),"%s",p);if(creating){char* slash=strrchr(parent,'/');if(!slash)return 0;*slash=0;}if(!realpath(parent,resolved)||!allowed_path(resolved))return 0;if(!creating&&strcmp(resolved,p))return 0;struct stat st;if(!lstat(p,&st)&&S_ISLNK(st.st_mode))return 0;return 1;}
static void file_command(int fd,uint32_t op,uint32_t size){struct FileCommand q;if(size<sizeof(q)||exact(fd,&q,sizeof(q)))return;
 if(q.length>1024*1024||q.offset>1024ULL*1024*1024*1024||!checked_path(q.path,op==12||op==13)){write_all(fd,"FILE ERROR invalid path or bounds\n",34);return;}
 if(op==11){if(size!=sizeof(q))return;int f=open(q.path,O_RDONLY|O_NOFOLLOW);struct stat st;if(f<0||fstat(f,&st)||!S_ISREG(st.st_mode)||(uint64_t)st.st_size<q.offset){if(f>=0)close(f);return;}uint32_t n=(uint64_t)st.st_size-q.offset<q.length?(uint64_t)st.st_size-q.offset:q.length;char* b=malloc(n?n:1);if(b&&pread(f,b,n,q.offset)==n){char h[80];int hn=snprintf(h,sizeof(h),"FILE DATA %u %llu\n",n,(unsigned long long)st.st_size);write_all(fd,h,hn);write_all(fd,b,n);}free(b);close(f);return;}
 char stage[1100];snprintf(stage,sizeof(stage),"%s.devbridge-part",q.path);
 if(op==12){if(size!=sizeof(q)+q.length||q.flags>1)return;char* b=malloc(q.length?q.length:1);if(!b||exact(fd,b,q.length)){free(b);return;}int f=open(stage,O_WRONLY|O_CREAT|O_NOFOLLOW|(q.flags?O_TRUNC:0),0600);int okay=f>=0&&pwrite(f,b,q.length,q.offset)==q.length&&fsync(f)==0;if(f>=0)close(f);free(b);write_all(fd,okay?"FILE OK\n":"FILE ERROR write\n",okay?8:17);return;}
 if(op==13){if(size!=sizeof(q)||q.flags>1)return;struct stat st;if(lstat(stage,&st)||!S_ISREG(st.st_mode)||(uint64_t)st.st_size!=q.offset||(!q.flags&&!lstat(q.path,&st))){write_all(fd,"FILE ERROR commit refused\n",26);return;}if(chmod(stage,strstr(q.path,".devbridge-stage/")?0755:0644)){write_all(fd,"FILE ERROR permissions\n",23);return;}int rc=rename(stage,q.path);write_all(fd,rc?"FILE ERROR rename\n":"FILE OK\n",rc?18:8);}
}
