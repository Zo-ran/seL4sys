cmake_minimum_required(VERSION 3.7.2)

project(shell C)

add_executable(shell 
    src/shell.c
    src/syscall.c
)
# target_include_directories( PRIVATE include)
target_link_libraries(shell sel4runtime muslc sel4 sel4muslcsys)

# warn about everything
add_compile_options(-Wall -Werror -W -Wextra)

add_app(shell)