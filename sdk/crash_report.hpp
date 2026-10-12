// SPDX-License-Identifier: GPL-3.0-or-later
// (AI-assisted) Opt-in app crash reporting. Unwinding after a crash is best effort.
#pragma once
#include <signal.h>
#include <ucontext.h>
#include <unwind.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdint>
#include <initializer_list>
namespace devbridge_crash {
inline int report_fd=-1;inline volatile sig_atomic_t reporting=0;inline unsigned frames=0;inline int probe_pipe[2]={-1,-1};
inline void line(const char* label,uint64_t value){char b[96];unsigned n=0;while(*label&&n<60)b[n++]=*label++;b[n++]=' ';b[n++]='0';b[n++]='x';for(int i=15;i>=0;i--)b[n++]="0123456789abcdef"[(value>>(i*4))&15];b[n++]='\n';if(report_fd>=0)(void)write(report_fd,b,n);}
inline _Unwind_Reason_Code frame(_Unwind_Context* context,void*){if(frames++>=32)return _URC_END_OF_STACK;line("FRAME",_Unwind_GetIP(context));return _URC_NO_REASON;}
inline void handler(int sig,siginfo_t* info,void* context){if(reporting)_exit(128+sig);reporting=1;line("SIGNAL",sig);line("FAULT",(uintptr_t)info->si_addr);auto* uc=(ucontext_t*)context;line("RIP",uc->uc_mcontext.mc_rip);line("RSP",uc->uc_mcontext.mc_rsp);line("RBP",uc->uc_mcontext.mc_rbp);
 // Kernel copy-in through a pipe validates frame memory without direct dereferencing.
 uintptr_t cursor=uc->uc_mcontext.mc_rbp,lower=uc->uc_mcontext.mc_rsp;
 for(unsigned i=0;i<32&&probe_pipe[0]>=0;i++){if(cursor<lower||cursor-lower>1024*1024||cursor%8)break;uint64_t pair[2]{};if(write(probe_pipe[1],(const void*)cursor,sizeof(pair))!=sizeof(pair))break;if(read(probe_pipe[0],pair,sizeof(pair))!=sizeof(pair))break;line("STACK_RETURN",pair[1]);if(pair[0]<=cursor)break;cursor=pair[0];}
 frames=0;_Unwind_Backtrace(frame,nullptr);if(report_fd>=0)close(report_fd);_exit(128+sig);}
inline bool install(const char* report_path){report_fd=open(report_path,O_WRONLY|O_CREAT|O_APPEND|O_NOFOLLOW,0600);if(report_fd<0)return false;line("SESSION_PID",getpid());if(pipe(probe_pipe))return false;struct sigaction action{};action.sa_sigaction=handler;action.sa_flags=SA_SIGINFO|SA_RESETHAND;sigemptyset(&action.sa_mask);for(int sig:{SIGSEGV,SIGBUS,SIGILL,SIGFPE,SIGABRT})if(sigaction(sig,&action,nullptr))return false;return true;}
}
