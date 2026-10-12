/* Copyright (C) 2024 John Törnblom

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#include <ps5/kernel.h>

#include "elfldr.h"
#include "log.h"
#include "notify.h"
#include "selfldr.h"
#include "uri.h"


#include <elf.h>
#include <time.h>
#include <sys/stat.h>
#include "bridge_config.h"
#include "asset.h"
INCASSET(browser_open_elf,"browser-open.elf");
INCASSET(controller_service_elf,"controller-service.elf");
INCASSET(package_elf,"package.elf");
INCASSET(apps_elf,"apps.elf");
INCASSET(photo_list_elf,"photo-list.elf");
INCASSET(photo_read_elf,"photo-read.elf");
static void event(const char* message) {
 mkdir("/data/ps5-devbridge",0777);
 FILE* f=fopen("/data/ps5-devbridge/events.log","a");
 if(f){fprintf(f,"[devbridge] %s\n",message);fclose(f);}
}
static void local_address(char* ip,size_t size){
 snprintf(ip,size,"unavailable");struct ifaddrs* interfaces=NULL;
 if(getifaddrs(&interfaces)!=0)return;
 for(struct ifaddrs* i=interfaces;i;i=i->ifa_next){
  if(!i->ifa_addr||i->ifa_addr->sa_family!=AF_INET||!strncmp(i->ifa_name,"lo",2))continue;
  struct sockaddr_in* address=(void*)i->ifa_addr;
  if(address->sin_addr.s_addr!=0)inet_ntop(AF_INET,&address->sin_addr,ip,size);
 }
 freeifaddrs(interfaces);
}
static int exact(int fd,void* data,size_t size){
 char* p=data;
 while(size){ssize_t n=read(fd,p,size);if(n<=0)return -1;p+=n;size-=n;}return 0;
}
static int safe_elf(const uint8_t* b,size_t n){
 if(n<sizeof(Elf64_Ehdr))return -1;
 const Elf64_Ehdr* h=(const void*)b;
 if(memcmp(b,"\177ELF",4)||h->e_ident[EI_CLASS]!=ELFCLASS64||h->e_ident[EI_DATA]!=ELFDATA2LSB||h->e_machine!=EM_X86_64||h->e_phentsize!=sizeof(Elf64_Phdr)||!h->e_phnum||h->e_phnum>128)return -1;
 if(h->e_phoff>n || h->e_phnum>(n-h->e_phoff)/sizeof(Elf64_Phdr))return -1;
 if(h->e_shoff>n || h->e_shnum>(n-h->e_shoff)/sizeof(Elf64_Shdr))return -1;
 uint64_t high=0;int entry=0;
 for(unsigned i=0;i<h->e_phnum;i++){
  const Elf64_Phdr* p=(const void*)(b+h->e_phoff+i*sizeof(*p));
  if(p->p_offset>n||p->p_filesz>n-p->p_offset||p->p_filesz>p->p_memsz||p->p_vaddr>128*1024*1024||p->p_memsz>128*1024*1024-p->p_vaddr)return -1;
  if(p->p_type==PT_LOAD){if(p->p_vaddr+p->p_memsz>high)high=p->p_vaddr+p->p_memsz;if((p->p_flags&PF_X)&&h->e_entry>=p->p_vaddr&&h->e_entry-p->p_vaddr<p->p_memsz)entry=1;}
 }
 if(h->e_type!=ET_DYN || !high || !entry)return -1;
 for(unsigned i=0;i<h->e_shnum;i++){
  const Elf64_Shdr* sh=(const void*)(b+h->e_shoff+i*sizeof(*sh));
  if(sh->sh_type!=SHT_NOBITS&&(sh->sh_offset>n||sh->sh_size>n-sh->sh_offset))return -1;
  if(sh->sh_type==SHT_RELA){if(sh->sh_size%sizeof(Elf64_Rela))return -1;for(size_t j=0;j<sh->sh_size/sizeof(Elf64_Rela);j++){
   const Elf64_Rela* r=(const void*)(b+sh->sh_offset+j*sizeof(*r));
   if((uint32_t)r->r_info==R_X86_64_RELATIVE && (r->r_offset>high||sizeof(uint64_t)>high-r->r_offset))return -1;
  }}
 }
 return 0;
}
// (AI-assisted) Relay commands to a resident authenticated localhost worker.
static int input_connect(void){int s=socket(AF_INET,SOCK_STREAM,0);if(s<0)return -1;struct sockaddr_in a={0};a.sin_len=sizeof(a);a.sin_family=AF_INET;a.sin_port=htons(1343);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(connect(s,(void*)&a,sizeof(a))){close(s);return -1;}return s;}
static int write_all(int fd,const void* data,size_t n){const char* p=data;while(n){ssize_t got=write(fd,p,n);if(got<=0)return -1;p+=got;n-=got;}return 0;}
static void input_relay(int fd,const char* header,uint32_t size){
 unsigned char body[524];if(exact(fd,body,size))return;
 int s=input_connect();if(s<0){char* args[]={"devbridge-input.elf",NULL};if(elfldr_spawn(-1,args,controller_service_elf,controller_service_elf_size)<0){write_all(fd,"INPUT ERROR worker spawn\n",25);return;}
 for(unsigned i=0;i<100&&s<0;i++){usleep(50000);s=input_connect();}}
 if(s<0){write_all(fd,"INPUT ERROR worker unavailable\n",31);return;}
 struct timeval wait={40,0};setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&wait,sizeof(wait));
 if(!write_all(s,header,80)&&!write_all(s,body,size)){char b[4096];ssize_t n;while((n=read(s,b,sizeof(b)))>0)if(write_all(fd,b,n))break;}
 close(s);
}
#include "file_commands.h"
static time_t last_host=0;
static void on_connection(int fd,int remote){
 struct timeval timeout={5,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
 setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&(int){1},sizeof(int));
 char header[80];if(exact(fd,header,sizeof(header)))return;
 unsigned diff=0;for(int i=0;i<64;i++)diff|=(unsigned char)header[8+i]^(unsigned char)BRIDGE_KEY[i];
 if(memcmp(header,"PS5DEV1\0",8)||diff){event("Rejected authentication");return;}
 if(remote)last_host=time(NULL);
 uint32_t op,size;memcpy(&op,header+72,4);memcpy(&size,header+76,4);
 if(op==20&&size==1044){char* args[]={"managed-app.elf",NULL};elfldr_spawn(fd,args,apps_elf,apps_elf_size);return;}
 if(op>=11&&op<=13){pid_t child=fork();if(child==0){file_command(fd,op,size);close(fd);_exit(0);}return;}
 if((op==14||op==15||op==19)&&size==1028){char* args[]={"package-command.elf",NULL};elfldr_spawn(fd,args,package_elf,package_elf_size);return;}
 if(op==16&&size==0){char* args[]={"photo-list.elf",NULL};elfldr_spawn(fd,args,photo_list_elf,photo_list_elf_size);return;}
 if(op==17&&size==776){char* args[]={"photo-read.elf",NULL};elfldr_spawn(fd,args,photo_read_elf,photo_read_elf_size);return;}
 if(op==10&&size>=28&&size<=524){pid_t child=fork();if(child==0){input_relay(fd,header,size);close(fd);_exit(0);}if(child<0)event("Input relay fork failed");return;}
 if(op==1&&size==0){char ip[INET_ADDRSTRLEN],reply[256];local_address(ip,sizeof(ip));int n=snprintf(reply,sizeof(reply),"DEVBRIDGE/1 READY version=0.2.0 port=1337 ip=%s host=%s age=%lld firmware=%08x\n",ip,last_host&&time(NULL)-last_host<15?"connected":"idle",last_host?(long long)(time(NULL)-last_host):-1LL,(unsigned)kernel_get_fw_version());write(fd,reply,n);return;}
 if(op==3&&size==0){FILE* f=fopen("/data/ps5-devbridge/events.log","r");if(f){fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,n>16384?n-16384:0,SEEK_SET);char line[512];while(fgets(line,sizeof(line),f))write(fd,line,strlen(line));fclose(f);}return;}
 if(op==5&&size==0){FILE* f=fopen("/data/ps5-devbridge/browser-request.txt","r");if(f){char url[2048]={0};if(fgets(url,sizeof(url),f))write(fd,url,strlen(url));fclose(f);remove("/data/ps5-devbridge/browser-request.txt");}return;}
 if((op==4||op==9)&&size>0&&size<2048){char url[2048]={0};if(exact(fd,url,size))return;if(strncmp(url,"https://",8)&&strncmp(url,"http://",7))return;if(strpbrk(url,"\r\n"))return;
  char* argv[]={"dev-browser.elf",url,NULL};event("Launching PS5 browser helper");
  if(elfldr_spawn(fd,argv,browser_open_elf,browser_open_elf_size)<0)event("Browser helper spawn failed");return;
 }
 if(op==8&&size==0){FILE* f=fopen("/data/ps5-devbridge/stop-input","w");if(f){fputs("stop",f);fclose(f);}event("Stop input requested");write(fd,"DEVBRIDGE/1 STOP REQUESTED\n",26);return;}
 if(op==7&&size>0&&size<=8192){char message[8193]={0};if(exact(fd,message,size))return;for(uint32_t i=0;i<size;i++)if((unsigned char)message[i]<32 && message[i]!='\n' && message[i]!='\t')message[i]=' ';event(message);write(fd,"DEVBRIDGE/1 LOGGED\n",18);return;}

 if(op!=2||size<64||size>8*1024*1024){event("Rejected command or payload size");return;}
 uint8_t* buf=malloc(size);if(!buf)return;
 if(exact(fd,buf,size)||safe_elf(buf,size)){event("Rejected incomplete or invalid ELF");free(buf);return;}
 char* argv[]={"dev-test.elf",NULL};event("Launching uploaded ELF (AI-assisted)");
 pid_t pid=elfldr_spawn(fd,argv,buf,size);
 char line[128];snprintf(line,sizeof(line),"Spawn result pid=%d (spawn is not test success)",pid);event(line);
 if(pid<0)write(fd,"DEVBRIDGE/1 ERROR spawn\n",strlen("DEVBRIDGE/1 ERROR spawn\n"));
 free(buf);
}

/**
 * Serve ELF loader via a socket.
 **/
static int
serve_elfldr(uint16_t port) {
  struct sockaddr_in srvaddr;
  struct sockaddr_in cliaddr;
  socklen_t socklen;
  int connfd;
  int srvfd;

  if((srvfd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    LOG_PERROR("socket");
    return -1;
  }

  if(setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    LOG_PERROR("setsockopt");
    return -1;
  }

  memset(&srvaddr, 0, sizeof(srvaddr));
  srvaddr.sin_family = AF_INET;
  srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  srvaddr.sin_port = htons(port);

  if(bind(srvfd, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) != 0) {
    LOG_PERROR("bind");
    return -1;
  }

  if(listen(srvfd, 5) != 0) {
    LOG_PERROR("listen");
    return -1;
  }

  while(1) {
    socklen = sizeof(cliaddr);
    if((connfd=accept(srvfd, (struct sockaddr*)&cliaddr, &socklen)) < 0) {
      LOG_PERROR("accept");
      break;
    }

    if(cliaddr.sin_addr.s_addr != inet_addr(BRIDGE_PEER) && cliaddr.sin_addr.s_addr != htonl(INADDR_LOOPBACK)){close(connfd);event("Rejected peer address");continue;}
    on_connection(connfd,cliaddr.sin_addr.s_addr!=htonl(INADDR_LOOPBACK));
    close(connfd);
  }

  return close(srvfd);
}


static int
notify_address(const char* prefix, int port) {
  char ip[INET_ADDRSTRLEN] = "127.0.0.1";
  struct ifaddrs *ifaddr;

  if(getifaddrs(&ifaddr) == -1) {
    LOG_PERROR("getifaddrs");
    return -1;
  }

  // Enumerate all AF_INET IPs
  for(struct ifaddrs *ifa=ifaddr; ifa!=NULL; ifa=ifa->ifa_next) {
    if(ifa->ifa_addr == NULL) {
      continue;
    }

    if(ifa->ifa_addr->sa_family != AF_INET) {
      continue;
    }

    // skip localhost
    if(!strncmp("lo", ifa->ifa_name, 2)) {
      continue;
    }

    struct sockaddr_in *in = (struct sockaddr_in*)ifa->ifa_addr;
    inet_ntop(AF_INET, &(in->sin_addr), ip, sizeof(ip));

    // skip interfaces without an ip
    if(!strncmp("0.", ip, 2)) {
      continue;
    }
  }

  freeifaddrs(ifaddr);

  unsigned fw=kernel_get_fw_version();
  notify("%s %s:%d\nFirmware %02x.%02x", prefix, ip, port, fw>>24, (fw>>16)&255);
  LOG_PRINTF("%s %s:%d\n", prefix, ip, port);

  return 0;
}


/**
 *
 **/
int main() {
  int port = BRIDGE_PORT;
  pid_t pid;

  LOG_PRINTF("Socket server was compiled at %s %s\n", __DATE__, __TIME__);

  if(chdir("/")) {
    LOG_PERROR("chdir");
    return -1;
  }

  syscall(SYS_setsid);
  while((pid=elfldr_find_pid("devbridge.elf")) > 0) {
    if(kill(pid, SIGKILL)) {
      LOG_PERROR("kill");
      _exit(-1);
    }
    sleep(1);
  }

  syscall(SYS_thr_set_name, -1, "devbridge.elf");
  signal(SIGCHLD, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  event("PS5 DevBridge v0.2.0 started on port 1337 (AI-assisted)");
  notify_address("Welcome to PS5 DevBridge v0.2.0 (AI-assisted)\nPayload ready - IP / port:", port);
  while(1) {
    serve_elfldr(port);
    sleep(3);
  }

  return 0;
}
