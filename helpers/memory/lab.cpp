#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <ctime>
struct Data {uint32_t marker,health;float energy;uint32_t coins;uint64_t pointer1,pointer2;};
int main(){
 syscall(SYS_thr_set_name,-1,"db-memory-lab");
 auto* data=(Data*)mmap(nullptr,16384,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);if(data==MAP_FAILED)return 1;
 data->marker=0x71cc1122;data->health=100;data->energy=42.5f;data->coins=25;data->pointer1=(uint64_t)&data->health;data->pointer2=(uint64_t)&data->pointer1;
 unlink("/data/ps5-devbridge/memory-lab.stop");
 FILE*f=fopen("/data/ps5-devbridge/memory-lab.json","w");if(!f)return 1;
 fprintf(f,"{\"pid\":%d,\"base\":%llu,\"size\":16384,\"health\":%llu,\"energy\":%llu,\"coins\":%llu,\"pointer1\":%llu,\"pointer2\":%llu}\n",getpid(),(unsigned long long)data,(unsigned long long)&data->health,(unsigned long long)&data->energy,(unsigned long long)&data->coins,(unsigned long long)&data->pointer1,(unsigned long long)&data->pointer2);fclose(f);
 for(int i=0;i<6000&&access("/data/ps5-devbridge/memory-lab.stop",F_OK)!=0;i++)usleep(100000);
 munmap(data,16384);return 0;
}
