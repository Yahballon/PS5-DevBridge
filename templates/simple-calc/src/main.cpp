// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) Native calculator starter using ps5-homebrew-ui.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "crash_report.hpp"
#include <array>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cmath>
static std::string read(const char* path){FILE* f=fopen(path,"rb");if(!f)return {};fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<1||n>16000000){fclose(f);return {};}std::string s(n,0);fread(s.data(),1,s.size(),f);fclose(f);return s;}
int main(){mkdir("/data/ps5-devbridge",0755);mkdir("/data/ps5-devbridge/crashes",0755);devbridge_crash::install("/data/ps5-devbridge/crashes/PPSA99339.txt");
 hui::ps5::Display display;if(!display.open())hui::sys::park();hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();hui::gfx::Font font;if(!font.load(read("/app0/assets/fonts/inter-semibold.huifont")))hui::sys::park();unsigned atlas=renderer.batch().create_font_texture(font);hui::ps5::Pad pad;pad.open();
 std::string input="0",notice="D-pad: select | X: enter | Circle: exit";double stored=0;char operation=0;bool fresh=true;int selection=0;unsigned previous=0;
 const char* keys[]={"7","8","9","/","4","5","6","*","1","2","3","-","C","0","=","+"};
 for(;;){std::array<hui::PadSample,64> samples{};unsigned pressed=0;size_t count=pad.read(samples);for(size_t i=0;i<count;i++){pressed|=samples[i].buttons&~previous;previous=samples[i].buttons;}
 if(pressed&hui::pad_bits::kCircle){pad.close();renderer.release();display.close();hui::sys::quit();}
 if(pressed&hui::pad_bits::kLeft)selection=(selection+15)%16;if(pressed&hui::pad_bits::kRight)selection=(selection+1)%16;if(pressed&hui::pad_bits::kUp)selection=(selection+12)%16;if(pressed&hui::pad_bits::kDown)selection=(selection+4)%16;
 if(pressed&hui::pad_bits::kCross){char key=keys[selection][0];if(key>='0'&&key<='9'){if(fresh||input=="0")input="";if(input.size()<14)input+=key;fresh=false;}else if(key=='C'){input="0";stored=0;operation=0;fresh=true;}else if(key=='='){double rhs=strtod(input.c_str(),nullptr),value=rhs;bool valid=true;switch(operation){case '+':value=stored+rhs;break;case '-':value=stored-rhs;break;case '*':value=stored*rhs;break;case '/':if(rhs==0)valid=false;else value=stored/rhs;break;}char b[64];snprintf(b,sizeof(b),"%.10g",value);input=valid&&std::isfinite(value)?b:"Error";fresh=true;operation=0;}else{stored=strtod(input.c_str(),nullptr);operation=key;fresh=true;}}
 using namespace hui::gfx;DrawList ui;ui.gradient_rect({0,0,1920,1080},0,Color::rgb(0x071225),Color::rgb(0x020711));ui.text(font,atlas,"SIMPLE CALC",960,150,54,Color::rgb(0x4edcff),Align::center);ui.bordered_rect({540,210,840,120},20,Color::rgb(0x101f35),2,Color::rgb(0x317ca3));ui.text(font,atlas,input,1340,290,48,Color::rgb(0xffffff),Align::right);
 for(int i=0;i<16;i++){Rect button{540.0f+(i%4)*215,370.0f+(i/4)*130,195,110};ui.bordered_rect(button,18,Color::rgb(i==selection?0x164c71:0x142038),i==selection?3:1,Color::rgb(i==selection?0x52ddff:0x2c4160));ui.text(font,atlas,keys[i],button.x+97,button.y+72,40,Color::rgb(0xffffff),Align::center);}ui.text(font,atlas,notice,960,1025,24,Color::rgb(0x9cb7d2),Align::center);renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);}
}
