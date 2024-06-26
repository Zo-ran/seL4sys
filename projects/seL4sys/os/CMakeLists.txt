cmake_minimum_required(VERSION 3.7.2)

project(os C)

# list of apps to include in the cpio archive
get_property(apps GLOBAL PROPERTY apps_property)
include(cpio)
MakeCPIO(archive.o "${apps}")

add_executable(
    os 
    src/root.c
    src/process.c
    src/timer/timer.c
    src/syscall_handler.c
    src/console/vga.c
    src/console/keyboard.c
    src/console/stdio_handler.c
    src/filesystem/disk.c
    src/filesystem/ext.c
    src/filesystem/fs.c
    src/filesystem/pagefile.c
    src/vm/vmfault_handler.c
    src/vm/mem_projection.c
    src/worker/sys_worker.c
    archive.o
)

target_include_directories(os PRIVATE include)
target_link_libraries(
    os
    sel4runtime sel4
    muslc utils 
    sel4muslcsys sel4platsupport sel4utils sel4debug 
    sel4allocman sel4sync
)

# Tell the build system that this application is the root task. 
include(rootserver)
DeclareRootserver(os)

add_compile_options(-Wall -Werror -W -Wextra)