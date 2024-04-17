cmake_minimum_required(VERSION 3.7.2)

project(libsyscall C)

add_library(
    libsyscall 
    src/sos.c
    src/sys_exit.c
    src/sys_morecore.c
    src/sys_stdio.c
    src/sys_thread.c
    src/sys_time.c
    src/init.c
)
target_include_directories(libsyscall PUBLIC include)
target_link_libraries(libsyscall sel4runtime muslc sel4 utils sel4muslcsys)
