// (AI-assisted) Read-only bounded Store download diagnostics.
#include <cstdio>
#include <cstring>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
int main(){const char* root="/data/jb-store/logs";DIR* d=opendir(root);if(!d){puts("No Store logs directory");return 1;}unsigned files=0;while(auto* e=readdir(d)){std::string name=e->d_name;if(name.find('/')!=name.npos||!name.ends_with(".log")||files++>=64)continue;FILE* f=fopen((std::string(root)+"/"+name).c_str(),"rb");if(!f)continue;fseek(f,0,SEEK_END);long n=ftell(f);if(n<0){fclose(f);continue;}fseek(f,n>32768?n-32768:0,SEEK_SET);printf("STORELOG %s\n",name.c_str());char block[4096];size_t count;while((count=fread(block,1,sizeof(block),f)))fwrite(block,1,count,stdout);fclose(f);puts("\nSTORELOG END");}closedir(d);return 0;}
