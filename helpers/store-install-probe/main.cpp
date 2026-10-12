// (AI-assisted) Read-only Store installation location probe.
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
int main(){for(const char* root:{"/data/homebrew","/mnt/usb0/homebrew","/mnt/usb1/homebrew","/mnt/usb2/homebrew","/mnt/usb3/homebrew","/mnt/shadowmnt","/data"}){
 DIR* d=opendir(root);if(!d)continue;printf("ROOT %s\n",root);
 while(auto* e=readdir(d)){if(e->d_name[0]=='.')continue;std::string path=std::string(root)+"/"+e->d_name;
 if(strstr(e->d_name,"FAKE00666")||strstr(e->d_name,"STORE")||strstr(e->d_name,"Store"))printf("CANDIDATE %s\n",path.c_str());
 std::string param=path+"/sce_sys/param.json";FILE* f=fopen(param.c_str(),"r");if(!f)continue;char buf[8192]{};fread(buf,1,sizeof(buf)-1,f);fclose(f);
 if(strstr(buf,"FAKE00666"))printf("STORE-PARAM %s\n%s\n",param.c_str(),buf);
 }closedir(d);
}return 0;}
