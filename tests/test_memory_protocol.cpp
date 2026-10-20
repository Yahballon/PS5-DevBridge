#include "../helpers/memory/protocol.hpp"
#include <cassert>
int main(){using namespace memory;
 Request r{kRequest,1,1,0,0,0,0,0,0,0};assert(valid(r,48));assert(!valid(r,47));
 r.command=3;r.pid=42;r.cookie=123;r.address=0x10000;r.length=4;assert(valid(r,48));
 auto bad=r;bad.cookie=0;assert(!valid(bad,48));bad=r;bad.address=0xffff800000000000ULL;assert(!valid(bad,48));
 bad=r;bad.address=0x7fffffffffffULL;assert(!valid(bad,48));bad=r;bad.length=kMaxRead+1;assert(!valid(bad,48));
 bad=r;bad.flags=1;assert(!valid(bad,48));bad=r;bad.count=1;assert(!valid(bad,48));
 r.command=4;r.flags=1;assert(valid(r,56));assert(!valid(r,55));r.length=17;assert(!valid(r,82));
 r.length=4;r.flags=0;assert(!valid(r,56));r.flags=1;r.reserved=1;assert(!valid(r,56));
}
