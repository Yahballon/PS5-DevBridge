// (AI-assisted) Native PS5 development console. SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "glyphs.hpp"
#include "browser_launcher.hpp"
#define BROWSER_HOME "https://example.com/"
#include <GL/glcorearb.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <dlfcn.h>
extern "C" int sceNetInit();
extern "C" int sceNetSocket(const char*,int,int,int);
extern "C" int sceNetConnect(int,const void*,unsigned);
extern "C" int sceNetSend(int,const void*,size_t,int);
extern "C" int sceNetRecv(int,void*,size_t,int);
extern "C" int sceNetSocketClose(int);
extern "C" int sceNetSetsockopt(int,int,int,const void*,unsigned);
struct Address{uint8_t size,family;uint16_t port;uint32_t ip;uint16_t vport;uint8_t zero[6];};
static std::string query(uint32_t op,const std::string& body=""){
 std::string key(64,0); // Public LAN mode, no pairing file.
 int s=sceNetSocket("devbridge_status",2,1,6);if(s<0)return "Network socket unavailable";
 int timeout=op==9?10000000:250000;for(int opt:{0x1105,0x1106,0x1109})sceNetSetsockopt(s,0xffff,opt,&timeout,sizeof(timeout));
 Address a{sizeof(Address),2,0x3905,0x0100007f,0,{}}; // 1337 in network byte order
 if(sceNetConnect(s,&a,sizeof(a))<0){sceNetSocketClose(s);return "Port 1337 offline - start devbridge-loader.elf";}
 std::array<char,80> h{};memcpy(h.data(),"PS5DEV1",7);memcpy(h.data()+8,key.data(),64);memcpy(h.data()+72,&op,4);uint32_t length=body.size();memcpy(h.data()+76,&length,4);
 size_t sent=0;while(sent<h.size()){int n=sceNetSend(s,h.data()+sent,h.size()-sent,0);if(n<=0)break;sent+=n;}
 size_t body_sent=0;while(body_sent<body.size()){int n=sceNetSend(s,body.data()+body_sent,body.size()-body_sent,0);if(n<=0)break;body_sent+=n;}
 std::string result;char b[1024];for(;;){int n=sceNetRecv(s,b,sizeof(b),0);if(n<=0)break;result.append(b,n);if(result.size()>16384)break;}
 sceNetSocketClose(s);return result.empty() && op!=5?"No response from port 1337":result;
}
static std::string endpoint(const std::string& status){
 auto at=status.find("ip=");if(at==std::string::npos)return "IP unavailable | PORT 1337";
 at+=3;auto end=status.find_first_of(" \r\n",at);return status.substr(at,end==std::string::npos?end:end-at)+" : 1337";
}
static int width,height;
static void rect(int x,int y,int w,int h,float r,float g,float b){
 glScissor(x*width/1920,(1080-y-h)*height/1080,w*width/1920,h*height/1080);glClearColor(r,g,b,1);glClear(GL_COLOR_BUFFER_BIT);
}
static void text(const std::string& str,int x,int y,int scale,float r=.7f,float g=.9f,float b=.8f){
 for(unsigned char c:str){if(c=='\n'){y+=scale*10;x=80;continue;}if(c>='a'&&c<='z')c-=32;if(c<32||c>95)c='?';auto rows=kGlyphs[c-32];for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(rows[row]&(16>>col))rect(x+col*scale,y+row*scale,scale,scale,r,g,b);x+=scale*6;if(x>1800){x=80;y+=scale*10;}if(y>1040)break;}
}
static std::string browser(const std::string& requested=BROWSER_HOME){
 return query(9,requested);
}
// (AI-assisted) Native app may send the embedded payload to an existing local loader.
static std::string start_payload(){
 const char* payload_path="/app0/devbridge-loader.elf";
 FILE* f=fopen(payload_path,"rb");if(!f)return "Embedded payload missing";fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<64||n>8*1024*1024){fclose(f);return "Invalid payload";}std::vector<char> b(n);bool okay=fread(b.data(),1,b.size(),f)==b.size();fclose(f);if(!okay)return "Payload read failed";
 int s=sceNetSocket("devbridge_bootstrap",2,1,6);if(s<0)return "Socket unavailable";int timeout=2000000;for(int opt:{0x1105,0x1106,0x1109})sceNetSetsockopt(s,0xffff,opt,&timeout,sizeof(timeout));Address a{sizeof(Address),2,0x3d23,0x0100007f,0,{}};
 if(sceNetConnect(s,&a,sizeof(a))<0){sceNetSocketClose(s);return "Local loader 9021 offline - load payload with Payload Manager";}size_t at=0;while(at<b.size()){int got=sceNetSend(s,b.data()+at,b.size()-at,0);if(got<=0)break;at+=got;}sceNetSocketClose(s);return at==b.size()?"Payload sent - waiting for port 1337":"Payload transfer failed";}
int main(){
 sceNetInit();hui::ps5::Display display;if(!display.open())hui::sys::park();width=display.width();height=display.height();
 hui::ps5::Pad pad;pad.open();glEnable(GL_SCISSOR_TEST);
 std::string status=query(1),host_status="AI / HOST: IDLE",log="PS5 DevBridge v0.2.2 (AI-assisted)\nDedicated ELF service: port 1337\nX: Open browser    Triangle: Refresh status    Options: Close app";
 uint32_t previous=0;uint64_t refresh=0;
 for(;;){
 std::array<hui::PadSample,64> samples{};size_t count=pad.read(samples);uint32_t pressed=0;for(size_t i=0;i<count;i++){pressed|=samples[i].buttons&~previous;previous=samples[i].buttons;}
 if(pressed&hui::pad_bits::kCross)status=browser();
 if(pressed&hui::pad_bits::kSquare){if(status.find("READY")!=std::string::npos)status="Payload already running";else status=start_payload();}
 if(pressed&hui::pad_bits::kOptions){pad.close();display.close();hui::sys::quit();}
 uint64_t now=hui::sys::monotonic_us();if((pressed&hui::pad_bits::kTriangle)||now>refresh){status=query(1);host_status=status.find("host=connected")!=std::string::npos?"AI / HOST: CONNECTED":"AI / HOST: IDLE";log=query(3);refresh=now+1000000;std::string requested=query(5);if(requested.rfind("http://",0)==0||requested.rfind("https://",0)==0)status=browser(requested);}
 // Browser launch is serviced only by this foreground app; no arbitrary shell commands.
 rect(0,0,1920,1080,.018f,.023f,.029f);rect(55,55,1810,135,.04f,.13f,.13f);
 text("PS5 DEVBRIDGE / LIVE DEVELOPMENT CONSOLE",80,80,4,.4f,1,.85f);
 text(endpoint(status),80,135,4,.85f,.95f,1);
 text(status.substr(0,96),80,205,2,1,.8f,.35f);
 text(host_status+" / LIVE LOGS",80,250,3,.4f,1,.85f);
 // Show the latest lines, rather than hiding new output below the TV edge.
 std::vector<std::string> lines;size_t at=0;while(at<log.size()){size_t end=log.find('\n',at);if(end==std::string::npos)end=log.size();lines.push_back(log.substr(at,end-at));at=end+1;}
 int y=290;for(size_t i=lines.size()>18?lines.size()-18:0;i<lines.size();i++){text(lines[i].substr(0,95),80,y,3);y+=34;}
 text("X BROWSER   SQUARE START PAYLOAD   TRIANGLE REFRESH   OPTIONS CLOSE",80,995,3,.6f,.7f,.8f);
 display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
