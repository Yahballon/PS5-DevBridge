// (AI-assisted) Registered titles must map staging memory through libkernel.
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

extern int sceKernelMmap(void *, size_t, int, int, int, int64_t, void **);
extern int sceKernelMunmap(void *, size_t);

static int native_errno(int result)
{
    // Native kernel errors carry the POSIX errno in the low 16 bits.
    if (((uint32_t)result & 0xffff0000u) == 0x80020000u)
        return (int)((uint32_t)result & 0xffffu);
    return EIO;
}

void *__wrap_mmap(void *address, size_t length, int protection, int flags,
                  int descriptor, off_t offset)
{
    void *mapped = NULL;
    int result = sceKernelMmap(address, length, protection, flags, descriptor,
                               (int64_t)offset, &mapped);
    if (result != 0) {
        errno = native_errno(result);
        return MAP_FAILED;
    }
    return mapped;
}

int __wrap_munmap(void *address, size_t length)
{
    int result = sceKernelMunmap(address, length);
    if (result != 0) {
        errno = native_errno(result);
        return -1;
    }
    return 0;
}

extern int sceKernelMprotect(void *, size_t, int);
int __wrap_mprotect(void *address, size_t length, int protection)
{
    int result = sceKernelMprotect(address, length, protection);
    if (result != 0) { errno = native_errno(result); return -1; }
    return 0;
}
