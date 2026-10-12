/*
 * ps5-native-app-boilerplate - Native application startup.
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Initializes the platform runtime, runs static constructors and main, and
 * hands normal process termination back to the platform runtime.
 */

#include <cstddef>
#include <cstdint>
#include <pthread.h>

using Destructor = void (*)();
using Initializer = void (*)();

extern "C"
{
    void _init_env(void *process_parameters);
    void jb_startup_mark(const char*) __attribute__((weak));
    int atexit(Destructor callback);
    [[noreturn]] void exit(int status);
    int application_main(int argc, char **argv, char **envp) __asm__("main");

    extern Initializer __preinit_array_start[] __attribute__((weak));
    extern Initializer __preinit_array_end[] __attribute__((weak));
    extern Initializer __init_array_start[] __attribute__((weak));
    extern Initializer __init_array_end[] __attribute__((weak));
    extern Initializer __fini_array_start[] __attribute__((weak));
    extern Initializer __fini_array_end[] __attribute__((weak));
}

namespace
{
struct MainArguments { int argc; char **argv; int status; };
void *run_application(void *opaque)
{
    auto *args = static_cast<MainArguments *>(opaque);
    if (jb_startup_mark) jb_startup_mark("v27: application thread entered with 16-MiB stack");
    args->status = application_main(args->argc, args->argv, nullptr);
    return nullptr;
}

void run_forward(Initializer *first, Initializer *last) noexcept
{
    if (first == nullptr || last == nullptr)
        return;
    while (first != last)
        (*first++)();
}

void run_reverse(Initializer *first, Initializer *last) noexcept
{
    if (first == nullptr || last == nullptr)
        return;
    while (last != first)
        (*--last)();
}
} // namespace

extern "C" __attribute__((weak)) void catchReturnFromMain(int status)
{
    (void)status;
}

extern "C" void _init()
{
    run_forward(__preinit_array_start, __preinit_array_end);
    run_forward(__init_array_start, __init_array_end);
}

extern "C" void _fini()
{
    run_reverse(__fini_array_start, __fini_array_end);
}

extern "C" [[noreturn]] __attribute__((visibility("default"))) void
_start(void *process_parameters, Destructor loader_teardown)
{
    const int argc = *static_cast<const int *>(process_parameters);
    auto *parameters = static_cast<std::uint8_t *>(process_parameters);
    auto **argv = reinterpret_cast<char **>(parameters + sizeof(std::uint64_t));

    if(jb_startup_mark)jb_startup_mark("v27: native entry reached; before libc init");
    _init_env(process_parameters);
    if(jb_startup_mark)jb_startup_mark("v27: libc init returned; before exit hooks");
    if (loader_teardown != nullptr)
        (void)atexit(loader_teardown);
    (void)atexit(_fini);
    if(jb_startup_mark)jb_startup_mark("v27: exit hooks ready; before constructors");
    _init();
    if(jb_startup_mark)jb_startup_mark("v27: constructors completed; before main");
    // Keep display, event loop and rendering on the same thread. Needs proper console testing.
    MainArguments args{argc, argv, 1};
    pthread_attr_t attr;
    int rc = pthread_attr_init(&attr);
    if (rc != 0) { if (jb_startup_mark) jb_startup_mark("v27: thread attribute initialization failed"); exit(1); }
    rc = pthread_attr_setstacksize(&attr, 16 * 1024 * 1024);
    pthread_t thread{};
    if (rc == 0) rc = pthread_create(&thread, &attr, run_application, &args);
    pthread_attr_destroy(&attr);
    if (rc != 0) { if (jb_startup_mark) jb_startup_mark("v27: application thread creation failed"); exit(1); }
    rc = pthread_join(thread, nullptr);
    if (rc != 0) exit(1);
    const int status = args.status;
    catchReturnFromMain(status);
    exit(status);
}
