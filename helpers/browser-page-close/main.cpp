// (AI-assisted) Serve one self-closing test page locally; no input, ptrace or kill calls.
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include "../../app/src/browser_launcher.hpp"
int main(){
 int server=socket(AF_INET,SOCK_STREAM,0);if(server<0)return 1;
 int reuse=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
 sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(1338);
 if(bind(server,(sockaddr*)&address,sizeof(address))||listen(server,1)){puts("PAGE-CLOSE server failed");close(server);return 1;}
 void* u=dlopen("libSceUserService.sprx",RTLD_LAZY);void* s=dlopen("libSceSystemService.sprx",RTLD_LAZY);
 store::browser_launch::Api api{u?reinterpret_cast<int(*)(void*)>(dlsym(u,"sceUserServiceInitialize")):nullptr,s?reinterpret_cast<int(*)(const char*,void*)>(dlsym(s,"sceSystemServiceLaunchWebBrowser")):nullptr,u?reinterpret_cast<int(*)()>(dlsym(u,"sceUserServiceTerminate")):nullptr};
 char message[256]{};int result=store::browser_launch::launch(api,"http://127.0.0.1:1338/close-test",message,sizeof(message));printf("PAGE-CLOSE browser_rc=%d %s\n",result,message);fflush(stdout);
 fd_set set;FD_ZERO(&set);FD_SET(server,&set);timeval wait{20,0};
 if(result!=0||select(server+1,&set,nullptr,nullptr,&wait)<=0){puts("PAGE-CLOSE no page request");close(server);return 1;}
 int client=accept(server,nullptr,nullptr);if(client<0){close(server);return 1;}
 timeval timeout{3,0};setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));int nosignal=1;setsockopt(client,SOL_SOCKET,SO_NOSIGPIPE,&nosignal,sizeof(nosignal));
 char request[2048]{};ssize_t count=recv(client,request,sizeof(request)-1,0);
 const char* html="<!doctype html><meta charset=utf-8><title>PS5 browser close test</title><style>body{background:#08141c;color:#80ffd5;font:28px sans-serif;padding:60px}</style><h1>Controller-free close test</h1><p id=s>Trying window.close()...</p><script>setTimeout(function(){window.close();setTimeout(function(){document.getElementById('s').textContent='The browser blocked automatic page closing. No system process was changed.';},1000);},1000);</script>";
 if(count>0&&!strncmp(request,"GET /close-test ",16)){
 char head[256];int n=snprintf(head,sizeof(head),"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",strlen(html));
 char response[2048];memcpy(response,head,n);memcpy(response+n,html,strlen(html));size_t left=n+strlen(html),at=0;while(left){ssize_t sent=send(client,response+at,left,0);if(sent<=0)break;at+=sent;left-=sent;}
 printf("PAGE-CLOSE test page sent bytes=%zu\n",at);
 }else puts("PAGE-CLOSE unexpected request; no content served");
 close(client);close(server);return 0;
}
