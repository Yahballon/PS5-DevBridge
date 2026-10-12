// (AI-assisted) Crashes only this disposable test payload, never a system process.
#include "../../sdk/crash_report.hpp"
__attribute__((noinline)) static void test_leaf(){raise(SIGABRT);}
__attribute__((noinline)) static void test_parent(){test_leaf();}
int main(){mkdir("/data/ps5-devbridge/crashes",0755);if(!devbridge_crash::install("/data/ps5-devbridge/crashes/devbridge-test.txt"))return 1;test_parent();return 0;}
