cmake_minimum_required(VERSION 3.7.2)

project(os C)

add_executable(
    os 
    src/root.c
    src/timer.c
    src/vga.c
    src/keyboard.c
)
target_include_directories(os PRIVATE include)
target_link_libraries(
    os
    sel4runtime sel4
    muslc utils 
    sel4muslcsys sel4platsupport sel4utils sel4debug 
    sel4allocman sel4keyboard
    libsyscall
)

# Tell the build system that this application is the root task. 
include(rootserver)
DeclareRootserver(os)

add_compile_options(-Wall -Werror -W -Wextra)