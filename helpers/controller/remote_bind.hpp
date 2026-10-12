#pragma once
static int trace(int request,pid_t target,void* address,int data){
 const pid_t self=getpid();const uint64_t saved=kernel_get_ucred_authid(self);
 uint8_t oldcaps[16]{},priv[16];memset(priv,0xff,sizeof(priv));
 if(!saved || kernel_get_ucred_caps(self,oldcaps)!=0)return -1;
 int rc=kernel_set_ucred_authid(self,0x4800000000010003ULL);
 if(rc==0)rc=kernel_set_ucred_caps(self,priv);
 if(rc==0)rc=(int)__syscall(SYS_ptrace,request,target,address,data);
 const int caps_rc=kernel_set_ucred_caps(self,oldcaps);
 const int auth_rc=kernel_set_ucred_authid(self,saved);
 uint8_t restored[16]{};bool okay=caps_rc==0 && auth_rc==0 && kernel_get_ucred_authid(self)==saved && kernel_get_ucred_caps(self,restored)==0 && !memcmp(oldcaps,restored,16);
 note("Debugger identity/capabilities restored",okay?0:-1);return okay?rc:-1;
}
static bool write_target(pid_t pid,uintptr_t address,const void* data,size_t size){
 ptrace_io_desc io{};io.piod_op=PIOD_WRITE_D;io.piod_offs=(void*)address;io.piod_addr=(void*)data;io.piod_len=size;
 return trace(PT_IO,pid,&io,0)==0 && io.piod_len==size;
}
static bool wait_stop(pid_t pid,int& status,unsigned limit){
 for(unsigned n=0;n<limit;n++){pid_t rc=waitpid(pid,&status,WNOHANG);if(rc==pid)return WIFSTOPPED(status);if(rc<0 && errno!=EINTR)return false;usleep(1000);}return false;
}

// (AI-assisted) Uses existing INT3 padding, never patches executable code.
static pid_t find_shellui(){
 int mib[]={1,14,8,0};size_t size=0;if(sysctl(mib,4,nullptr,&size,nullptr,0)<0 || !size || size>8*1024*1024)return -1;
 std::vector<unsigned char> data(size);if(sysctl(mib,4,data.data(),&size,nullptr,0)<0)return -1;
 for(size_t at=0;at+4<=size;){int length=0;std::memcpy(&length,data.data()+at,4);
  if(length<(int)(offsetof(kinfo_proc,ki_comm)+sizeof(kinfo_proc::ki_comm)) || (size_t)length>size-at)return -1;
  kinfo_proc p{};std::memcpy(&p,data.data()+at,std::min(sizeof(p),(size_t)length));at+=length;p.ki_comm[sizeof(p.ki_comm)-1]=0;
  if(p.ki_pid>0 && !std::strcmp(p.ki_comm,"SceShellUI"))return p.ki_pid;
 }return -1;
}
static int remote_bind(std::uint64_t id,std::uint32_t user) {
 const pid_t target=find_shellui();uint32_t pad=0,mbus=0;if(target<0)return -1;
 if(kernel_dynlib_handle(target,"libScePad.sprx",&pad)!=0 || !pad || kernel_dynlib_handle(target,"libSceMbus.sprx",&mbus)!=0)return -1;
 uintptr_t fn=kernel_dynlib_dlsym(target,mbus,"sceMbusBindDeviceWithUserId");
 uintptr_t padding=kernel_dynlib_dlsym(target,pad,"scePadGetDeviceId");unsigned char bytes[14]{};
 if(!fn || !padding || kernel_proc_copyout(target,padding+0x52,bytes,sizeof(bytes))!=0)return -1;
 for(auto b:bytes)if(b!=0xcc)return -1;
 uintptr_t trap=padding+0x52;
 int rc=trace(PT_ATTACH,target,nullptr,0);note("Remote bind attach",rc);if(rc!=0)return rc;
 int status=0;bool stopped=wait_stop(target,status,2000);int result=-1;
 struct reg saved{},regs{};bool changed=false;
 uintptr_t lower=0;std::vector<unsigned char> stack(16384);
 if(stopped && trace(PT_GETREGS,target,&saved,0)==0) {
  lower=(uintptr_t)saved.r_rsp-stack.size();
  if(kernel_proc_copyout(target,lower,stack.data(),stack.size())==0) {
   uintptr_t rsp=((uintptr_t)saved.r_rsp-256)&~uintptr_t(15);
   if(write_target(target,rsp,&trap,8)) {
    changed=true;regs=saved;regs.r_rsp=rsp;regs.r_rip=fn;regs.r_rdi=id;regs.r_rsi=user;regs.r_rdx=regs.r_rcx=regs.r_r8=regs.r_r9=0;regs.r_rax=0;
    if(trace(PT_SETREGS,target,&regs,0)==0 && trace(PT_CONTINUE,target,(void*)1,0)==0) {
     stopped=wait_stop(target,status,3000);
     if(!stopped){note("Remote bind timeout: rollback stop",-1);kill(target,SIGSTOP);stopped=wait_stop(target,status,2000);}
     if(stopped && WSTOPSIG(status)==SIGTRAP && trace(PT_GETREGS,target,&regs,0)==0 && (uintptr_t)regs.r_rip==trap+1)result=(int)regs.r_rax;
    }
   }
  } else note("Remote stack snapshot failed: no call",-1);
 }
 if(changed && stopped){bool restored=write_target(target,lower,stack.data(),stack.size());note("Remote full stack restored",restored?0:-1);int rr=trace(PT_SETREGS,target,&saved,0);note("Remote registers restored",rr);if(!restored || rr!=0)result=-1;}
 if(!stopped){note("CRITICAL: remote process not stopped for rollback",-1);result=-1;}
 int detached=trace(PT_DETACH,target,(void*)1,0);note("Remote bind detach",detached);return detached==0?result:-1;
}