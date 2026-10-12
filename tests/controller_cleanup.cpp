// (AI-assisted) Verify command reports and cleanup without controlling hardware.
#include "../helpers/controller/one_shot.hpp"
#include <cassert>
using namespace store::browser_close_test;
static bool closed,removed,restored,saw_command,fail;
static int controls;
static int open_device(){return 5;}
static int close_device(int){closed=true;return 0;}
static int assign(int,std::uint32_t){return 0;}
static void restore(){restored=true;}
static void sleep_ms(unsigned){}
static void log(const char*,int){}
static int control(int,unsigned long command,void* data){
 if(command==add_command){*static_cast<std::int32_t*>(static_cast<Request*>(data)->output)=123;return 0;}
 if(command==delete_command){removed=true;return 0;}
 assert(command==insert_command);controls++;
 auto* report=static_cast<unsigned char*>(static_cast<InsertRequest*>(data)->data);
 std::uint32_t buttons;std::memcpy(&buttons,report+0x0c,4);
 if(buttons==0x2000){saw_command=true;assert(report[0x10]==255&&report[0x14]==90);if(fail)return -77;}
 return 0;
}
int main(){
 Api api{open_device,control,close_device,sleep_ms,log,assign,restore};
 step_count=1;steps[0]={0x2000,100,32,255,128,128,128,90,0,{0,0}};
 assert(run(api,1)==0);assert(saw_command&&removed&&closed&&restored&&controls>120);
 closed=removed=restored=saw_command=false;fail=true;
 assert(run(api,1)==-4);assert(saw_command&&removed&&closed&&restored);
}
