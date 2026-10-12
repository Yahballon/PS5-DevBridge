APP_DEFINITIONS := GL_GLEXT_PROTOTYPES=1
APP_INCLUDE_PATHS := src .deps/ps5-opengl/current/include
APP_STATIC_ARCHIVES := .deps/ps5-opengl/libps5opengl-group.a
APP_IMPORT_STUBS := .deps/ps5-opengl/current/lib/libSceAgc.so .deps/ps5-opengl/current/lib/libSceAgcDriver.so
APP_WRAP_SYMBOLS := mmap munmap mprotect malloc calloc realloc free posix_memalign malloc_usable_size sceSystemServiceHideSplashScreen
APP_ROOT_FILES := build/devbridge-loader.elf
APP_ASSETS :=
include Makefile
