// (AI-assisted) Use the working JB STORE allocator during native startup and rendering.
// The previous OpenGL mspace heap could allocate before libc startup/privilege setup.
#include <stddef.h>
extern void* __real_malloc(size_t);
extern void* __real_calloc(size_t,size_t);
extern void* __real_realloc(void*,size_t);
extern void __real_free(void*);
extern int __real_posix_memalign(void**,size_t,size_t);
extern size_t __real_malloc_usable_size(const void*);
void* __wrap_malloc(size_t n){return __real_malloc(n);}
void* __wrap_calloc(size_t n,size_t s){return __real_calloc(n,s);}
void* __wrap_realloc(void* p,size_t n){return __real_realloc(p,n);}
void __wrap_free(void* p){__real_free(p);}
int __wrap_posix_memalign(void** p,size_t a,size_t n){return __real_posix_memalign(p,a,n);}
size_t __wrap_malloc_usable_size(const void* p){return __real_malloc_usable_size(p);}
void hui_heap_stats(size_t* live,size_t* peak,size_t* blocks,size_t* failures){if(live)*live=0;if(peak)*peak=0;if(blocks)*blocks=0;if(failures)*failures=0;}
